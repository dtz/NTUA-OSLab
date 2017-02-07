#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[]){
int fd ; 
	fd = open(".hello_there",O_RDONLY);
	if (fd < 0 ){
		perror("open");
		exit(EXIT_FAILURE);
	}
	dup2(fd,99);	
	execve("./riddle",NULL,NULL);
	return 0;

}
