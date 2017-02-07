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


#define BLOCK_SIZE      16
#define KEY_SIZE	16
#define BUF_SIZE	64
#define TCP_PORT    49842
#define TCP_BACKLOG 5

ssize_t insist_write(int fd, const void *buf, size_t cnt)
{
        ssize_t ret;
        size_t orig_cnt = cnt;
        
        while (cnt > 0) {
                ret = write(fd, buf, cnt);
                if (ret < 0)
                        return ret;
                buf += ret;
                cnt -= ret;
        }

        return orig_cnt;
}

ssize_t insist_read(int fd, void *buf, size_t cnt)
{
        ssize_t ret;
        size_t orig_cnt = cnt;

        while (cnt > 0) {
                ret = read(fd, buf, cnt);
                if (ret < 0)
                        return ret;
                if (ret == 0)
                        return (orig_cnt - cnt);
                buf += ret;
                cnt -= ret;
        }

        return orig_cnt;
}
	
int main () {

	char addrstr[INET_ADDRSTRLEN];
    	socklen_t len;
	char bufrd[BUF_SIZE],bufwr[BUF_SIZE];
	int sd, yes = 1, sd_max, curr_sd, cfd, ret, newsd;
    	struct sockaddr_in sa;

			
	ssize_t n;
	fd_set readfds;
		 struct timeval tv;
	
	/* Make sure a broken connection doesn't kill us */
    signal(SIGPIPE, SIG_IGN);

   	/* Create TCP/IP socket, used as main chat channel */
    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
       	perror("socket");
        exit(1);
 	}
 	fprintf(stderr, "Created TCP socket\n");

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
 	fprintf(stderr, "Incoming connection from %s:%d\n",addrstr, ntohs(sa.sin_port));
	
	 /* Clear the two sets */
    FD_ZERO(&readfds);

    /* Add the proper file descriptors to sets */
    FD_SET(0, &readfds);
    FD_SET(sd, &readfds);
    FD_SET(newsd, &readfds);

    /* Determine a timeout parameter */
    tv.tv_sec = 1000;
    tv.tv_usec = 0;

    for (;;) {
        FD_SET(0, &readfds);
        FD_SET(sd, &readfds);
    	FD_SET(newsd, &readfds);
        ret = select(newsd, &readfds, NULL, NULL, &tv);
        //printf("%d\n", ret);

        if (ret == -1) {
            perror("select");
            exit(1);
        }
        else if (ret == 0) {
            fprintf(stderr, "Timeout occured! No data after 100 seconds!\n");
        }
        else {
            if (FD_ISSET(0, &readfds)) {
                //fprintf(stderr, "Something to read...\n");
                n = read(0, bufrd, BUF_SIZE);
                //printf("read %d bytes\n", (int) n);
                if (n < 0) {
                    perror("read from stdin");
                    exit(1);
                }
                /* possible buffer overflow needs checking */
                bufrd[n] = '\0';

                n = insist_write(newsd, bufrd, n);
                printf("[local said]: %s", bufrd);
            }
			 if(FD_ISSET(newsd, &readfds)) {
                //fprintf(stderr, "\nRemote wrote something\n");
                n = read(newsd, bufwr, BUF_SIZE);
             	if (n < 0){
                	perror("read from remote peer failed");
					exit(1);
				}	
               	if (n == 0) {
					printf("Remote closed connection. Logging out...\n");
                	return -1;
            	}
                printf("%s", bufwr);
                memset(bufwr, '\0', BUF_SIZE);
            }
        }
    }
	return 0 ; 
}
