#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "server.h"
#include "client.h"

int main(int argc, char *argv[]) {
	char *mode = NULL;
	unsigned char *key = NULL;


	if ( argc < 3 ){
		fprintf(stderr, "Usage %s <server|client> <key>\n", argv[0]);
		exit(1);
	}

	mode = argv[1];
	key =  (unsigned char *) argv[2];

	printf("Welcome to OsLab chat room\n");

	if ( strcmp(mode, "server") == 0 ) {
		server(key);
	}
	else {
		char hostname[50];
		int port;
		printf("Please write the hostname you want to connect to: ");
		scanf("%s", hostname);
		printf("Please insert port's number: ");
		scanf("%d", &port);
		while (port > 65535 || port < 0) {
			printf("Port number invalid! Please insert port's number: ");
			scanf("%d", &port);
		}
		
		client(key, hostname, port);
	}
	
	fprintf(stderr, "Internal error. Unreachable point\n");

	return 1;
}
