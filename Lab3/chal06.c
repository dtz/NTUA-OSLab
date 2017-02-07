#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[]){
int pipeOne[2];
int pipeTwo[2];

	if (pipe(pipeOne) == -1){
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	if (pipe(pipeTwo) == -1){
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	dup2(pipeOne[0],33);	
	dup2(pipeOne[1],34);	
	dup2(pipeTwo[0],53);	
	dup2(pipeTwo[1],54);	
	execve("./riddle",NULL,NULL);
	return 0;

}
