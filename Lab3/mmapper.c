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
   fd = open(argv[1],O_RDWR);	
   if (!fd){
	  perror("open failed\n");
	  exit (-1);
   }
   lseek(fd,0x6f,SEEK_SET);
   write(fd,value,sizeof(value[0]));

    return 0; 

} 
