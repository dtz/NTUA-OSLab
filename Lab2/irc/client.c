#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>

#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <crypto/cryptodev.h>


#include "socket-common.h"
#include "client.h"
#include "io_ops.h"


#define BLOCK_SIZE      16
#define KEY_SIZE	16
#define FILENAME_SIZE   20

void client(unsigned char * key, char *hostname, int port) {

	int sd, ret, cfd, i, j;
	struct hostent *hp;
	char filename[FILENAME_SIZE];
	struct sockaddr_in sa; 
	ssize_t n;
	fd_set readfds, master;
	struct timeval tv;
	struct session_op sess;
        struct crypt_op cryp;
        struct {
                 unsigned char  in[BUF_SIZE],
                                encin[BUF_SIZE],
                                encout[BUF_SIZE],
                                decrypted[BUF_SIZE],
                                iv[BLOCK_SIZE];
        } data;
	
	/* Create TCP/IP socket, used as main chat channel */
	if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
   		perror("socket");
      		exit(1);
	}
	fprintf(stderr, "Created TCP socket\n");

	/* Look up remote hostname on DNS */
	if ( !(hp = gethostbyname(hostname))) {
      		printf("DNS lookup failed for host %s\n", hostname);
      		exit(1);
  	}

  	/* Connect to remote TCP port */
  	sa.sin_family = AF_INET;
   	sa.sin_port = htons(port);
  	memcpy(&sa.sin_addr.s_addr, hp->h_addr, sizeof(struct in_addr));
  	fprintf(stderr, "Connecting to remote host... "); fflush(stderr);
  	if (connect(sd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
      		perror("connect");
      		exit(1);
  	}
  	fprintf(stderr, "Connected.\n");

	memset(&sess, 0, sizeof(sess));
        memset(&cryp, 0, sizeof(cryp));
	memset(data.in,'\0', BUF_SIZE);
	memset(data.encin,'\0', BUF_SIZE);
	memset(data.encout,'\0', BUF_SIZE);
	memset(data.decrypted,'\0', BUF_SIZE);

	j = 1;
	do {
		snprintf(filename, 20, "/dev/cryptodev%d", j);
        	cfd = open(filename, O_RDWR);
		j++;
	} while ((cfd < 0) && (j < 32));


	if (j >= 32) {
		perror("open: no crypto-device available");
		exit(1);
	}

	if (fill_urandom_buf(data.iv, BLOCK_SIZE) < 0) {
                perror("getting data from /dev/urandom\n");
                exit(1);
        }

	/*
         * Get crypto session for AES128
         */
        sess.cipher = CRYPTO_AES_CBC;
        sess.keylen = KEY_SIZE;
        sess.key = key;
        if (ioctl(cfd, CIOCGSESSION, &sess)) {
                perror("ioctl(CIOCGSESSION)");
                exit(1);
        }

        /*
         * Encrypt data.in to data.encrypted
         */
        cryp.ses = sess.ses;
	cryp.len = sizeof(data.in);
        cryp.iv = data.iv;
	/* Clear the reading and master set */
	FD_ZERO(&master);
	FD_ZERO(&readfds);

    	/* Add the proper file descriptors to sets */
    	FD_SET(0, &master);
    	FD_SET(sd, &master);
    	/* Determine a timeout parameter */
    	tv.tv_sec = 100;
    	tv.tv_usec = 0;

    	for (;;) {
        	readfds = master;
        	ret = select(sd+1, &readfds, NULL, NULL, &tv);

        	if (ret == -1) {
            		perror("select");
            		exit(1);
        	}	
        	else if (ret == 0) {
            		fprintf(stderr, "Timeout occured! No data after 100 seconds!\n");
			printf("Connection with remote lost. Logging out...\n");
			goto out;
        	}
        	else {
            		if (FD_ISSET(0, &readfds)) {
                		n = read(0, data.in, BUF_SIZE);
                		if (n < 0) {
                    			perror("read from stdin");
                    			exit(1);
                		}
                		

                		if (strcmp((const char *)data.in,"logout\n") == 0)
                    			break;

				cryp.src = data.in;
	        		cryp.dst = data.encin;
        			cryp.op = COP_ENCRYPT;
        			if (ioctl(cfd, CIOCCRYPT, &cryp)) {
                			perror("ioctl(CIOCCRYPT)");
                			exit (1);
        			}	
            			n = insist_write(sd, data.encin, BUF_SIZE);
	        		memset(data.decrypted,'\0', BUF_SIZE);
	       			memset(data.in,'\0', BUF_SIZE);
            			memset(data.encin,'\0', BUF_SIZE);
       			}
			if(FD_ISSET(sd, &readfds)) {
                		n = insist_read(sd, data.encout, BUF_SIZE);
                		if (n < 0){
                    			perror("read from remote peer failed");
                    			exit(1);
                		}
                		if (n == 0) {
                    			printf("Remote closed connection. Logging out...\n");
                    			goto out;
                		}
				if (n != BUF_SIZE) {
					fprintf(stderr, "Unexpected short read!\n");
					exit(1);
				}

				cryp.src = data.encout;
        			cryp.dst = data.decrypted;
        			cryp.op = COP_DECRYPT;
        			if (ioctl(cfd, CIOCCRYPT, &cryp)) {
                			perror("ioctl(CIOCCRYPT)");
               				exit(1);
        			}	

       	 			for (i = 0; i < n; i++) {
                			printf("%c", data.decrypted[i]);
        			}	
                		memset(data.decrypted,'\0', BUF_SIZE);
                		memset(data.encout,'\0', BUF_SIZE);
            		}
        	}
    	}
 	printf("Logging out. Closing connection...\n");
out:
	if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
        	perror("ioctl(CIOCFSESSION)");
       		exit(1);
    	}
	
		
	if (close(sd) < 0) {
		perror("close");
		exit(1);
	}
        exit(0);
}
