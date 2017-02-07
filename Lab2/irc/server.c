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
#include "server.h"
#include "io_ops.h"


#define BLOCK_SIZE      16
#define KEY_SIZE	16


struct crypt_data{
	unsigned char  	in[BUF_SIZE],
			encin[BUF_SIZE],
			encout[BUF_SIZE],
			decrypted[BUF_SIZE],
			iv[BLOCK_SIZE];
};


void encrypt_data(int cfd , struct crypt_op * cryp , struct crypt_data * data){
	(*cryp).src = (*data).in;
	(*cryp).dst = (*data).encin;
	(*cryp).op = COP_ENCRYPT;
	if (ioctl(cfd, CIOCCRYPT, &(*cryp))) {
		perror("ioctl(CIOCCRYPT)");
		exit(1);
        }
}

	
void server(unsigned char * key) {

	char addrstr[INET_ADDRSTRLEN];
    	socklen_t len;
	int sd, yes = 1, sd_max, curr_sd, cfd, ret, newsd;

    	struct sockaddr_in sa;

	struct client *clients = NULL, *ptr = NULL, *ptr2 = NULL;
			
	ssize_t n;
	fd_set readfds, master;
	struct timeval tv;
	struct session_op sess;
        struct crypt_op cryp;
	struct crypt_data data ; 
	/* Make sure a broken connection doesn't kill us */
    	signal(SIGPIPE, SIG_IGN);
   	/* Create TCP/IP socket, used as main chat channel */
    	if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
       		perror("socket");
        	exit(1);
 	}
 	fprintf(stderr, "Created TCP socket\n");
	
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

 	/* Bind to a well-known port */
   	memset(&sa, 0, sizeof(sa));
  	sa.sin_family = AF_INET;
   	sa.sin_port = htons(TCP_PORT);
  	sa.sin_addr.s_addr = htonl(INADDR_ANY);
   	if (bind(sd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
     		perror("bind");
       		exit(1);
  	}
 	fprintf(stderr, "Bound TCP socket to port %d\n", TCP_PORT);

  	/* Listen for incoming connections */
 	if (listen(sd, TCP_BACKLOG) < 0) {
     		perror("listen");
      		exit(1);
    	}	

	memset(&sess, 0, sizeof(sess));
        memset(&cryp, 0, sizeof(cryp));
	memset(data.in,'\0', BUF_SIZE);
	memset(data.encin,'\0', BUF_SIZE);
	memset(data.encout,'\0', BUF_SIZE);
	memset(data.decrypted,'\0', BUF_SIZE);

        cfd = open("/dev/cryptodev0", O_RDWR);
	if (cfd < 0) {
		perror("open(/dev/cryptodev0)");
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

	cryp.ses = sess.ses;
	cryp.len = sizeof(data.in);
        cryp.iv = data.iv;
	sd_max = sd; 
	/* Clear the reading and master set */
	FD_ZERO(&master);
	FD_ZERO(&readfds);

    	/* Add the proper file descriptors to sets */
    	FD_SET(sd, &master);

	for (;;) {
		/* Determine a timeout parameter */
		tv.tv_sec = 100;
        	tv.tv_usec = 0;

		readfds = master;
        	ret = select(sd_max+1, &readfds, NULL, NULL, &tv);

        	if (ret == -1) {
            		perror("select");
            		exit(1);
        	}	
        	else if (ret == 0) {
            		fprintf(stderr, "Timeout occured! No data after 1000 seconds!\n");
			printf("Attention server shutting down!\n");
			goto out;
        	}
		else {
			if (FD_ISSET(sd, &readfds)) {
				fprintf(stderr, "Waiting for an incoming connection...\n");
				/* Accept an incoming connection */
   				len = sizeof(struct sockaddr_in);
   				if ((newsd = accept(sd, (struct sockaddr *)&sa, &len)) < 0) {
       					perror("accept");
      					exit(1);
  				}
				if (!inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr))) {
    					perror("could not format IP address");
       					exit(1);
  				}

				printf("[%s:%d] logged in.\n", addrstr, ntohs(sa.sin_port));
                                snprintf((char *) data.in, BUF_SIZE, "[%s:%d]: logged in.\n", addrstr, ntohs(sa.sin_port));
				/* encrypt data.in to data.encin */
				encrypt_data(cfd,&cryp,&data);

                                for (ptr = clients; ptr != NULL; ptr = ptr->next) {
                                        n = insist_write(ptr->sd, data.encin, BUF_SIZE);
                                }

				memset(data.in,'\0', BUF_SIZE);
                                memset(data.encin,'\0', BUF_SIZE);

				if (newsd > sd_max)
					sd_max = newsd;
				FD_SET(newsd, &master);
				clients = insert(clients, newsd, addrstr, sa.sin_port);
			}
			else {
				for (ptr = clients; ptr != NULL; ptr = ptr->next) {
					if (FD_ISSET(ptr->sd, &readfds)) {
						curr_sd = ptr->sd;
						n = insist_read(curr_sd, data.encout, BUF_SIZE);
                				if (n < 0){
                    					perror("read from remote peer failed");
                    					exit(1);
                				}
                				if (n == 0) {
                    					printf("[%s:%d] logged out.\n", ptr->addr, ntohs(ptr->port));
							snprintf((char *) data.in, BUF_SIZE, "[%s:%d]: logged out.\n", ptr->addr, ntohs(ptr->port));
								
							encrypt_data(cfd,&cryp,&data);

							for (ptr2 = clients; ptr2 != NULL; ptr2=ptr2->next) {
								if (curr_sd != ptr2->sd) {
									n = insist_write(ptr2->sd, data.encin, BUF_SIZE);
								}								
							}
							memset(data.in,'\0', BUF_SIZE);
                                                        memset(data.encin,'\0', BUF_SIZE);
						
							FD_CLR(curr_sd, &master);
							clients = delete(clients, curr_sd);
							if (close(curr_sd) < 0) {
								perror("close");
								exit(1);
							}
							continue;
                				}
						if (n != BUF_SIZE) {
                                        		fprintf(stderr, "Unexpected short read!\n");
                                        		exit(1);
                                		}

						snprintf((char *) data.in, BUF_SIZE, "[%s:%d]: ", ptr->addr, ntohs(ptr->port));	
						encrypt_data(cfd,&cryp,&data);
						
						for (ptr2 = clients; ptr2 != NULL; ptr2=ptr2->next) {
							n = insist_write(ptr2->sd, data.encin, BUF_SIZE);
							n = insist_write(ptr2->sd, data.encout, BUF_SIZE);								
						}
						memset(data.in,'\0', BUF_SIZE);
                                                memset(data.encin,'\0', BUF_SIZE);
                                                memset(data.encout, '\0', BUF_SIZE);						
					}
				}
			}
		}
	}

 	if (close(sd) < 0) {
      		perror("close");
		exit(1);	
	}

out:
	if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
        	perror("ioctl(CIOCFSESSION)");
       		exit(1);
    	}

	exit(0);
}
