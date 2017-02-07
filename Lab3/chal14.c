#include <stdlib.h> 
#include <stdio.h> 
#include <fcntl.h> 
#include <sys/mman.h> 
#include <sys/stat.h> 
#include <unistd.h> 

 

int main (int argc, char* argv[]) 

{
   pid_t pid ; 
   
   pid = getpid();
   while (pid !=32767){
	pid = fork();
        if (pid < 0 ){
		perror("fork\n");
                exit(-1);
        }
        if (pid == 0) {
	     if (getpid() == 32767){
		execve("riddle",NULL,NULL);
   	     }
	     exit(0);
        }
	printf("%d\n",pid);
  }
  return 0 ; 
} 
