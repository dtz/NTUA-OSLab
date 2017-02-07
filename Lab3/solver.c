#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[]){
int fd;
char filename[5];
int i ; 
/*	for (i= 0 ; i < 10 ; i++){
	snprintf(filename,5,"bf0%d",i);
	printf("%s\n",filename);
	fd = open(filename,O_RDWR);
		lseek(fd, 1073741824, 0);                                          
		write(fd,"21",16);
	}*/
	mkfifoat(33,"test",O_RDWR);
		return 0;

}
