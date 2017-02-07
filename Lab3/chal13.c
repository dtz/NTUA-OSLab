#include <stdlib.h> 
#include <stdio.h> 
#include <fcntl.h> 
#include <sys/mman.h> 
#include <sys/stat.h> 
#include <unistd.h> 

 

int main (int argc, char* argv[]) 

{
   int fd ; 
   char value[2] ;
   value[0]= *argv[2];
   fd = open(".hello_there",O_RDWR);	
   if (!fd){
	  perror("open failed\n");
	  exit (-1);
   }
   sleep(10);
   ftruncate(fd,32768);
   return 0; 
} 
