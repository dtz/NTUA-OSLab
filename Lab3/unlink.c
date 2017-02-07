#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

ssize_t insist_read(int fd, void *buf, size_t cnt)
{
        ssize_t ret;
        size_t orig_cnt = cnt;

        while (cnt > 0) {
                ret = read(fd, buf, cnt);
                   
                buf += ret;
                cnt -= ret;
		printf("cnt %d\n",cnt);

        }

        return orig_cnt;
}

int main(int argc, char *argv[]){
int fd;
char buffer[4096];
int i,j ; 
	
	fd = open("secret_number",O_RDWR);
	printf("fd is %d\n",fd);
	if (fd <2){
		perror("open\n");
		exit(-1);
	}
	i = insist_read(fd,buffer,4096);
	
	printf("%s\n",buffer);
	return 0;

}
