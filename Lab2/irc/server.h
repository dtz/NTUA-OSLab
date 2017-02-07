#ifndef _SERVER_H
#define _SERVER_H

#include <arpa/inet.h>

struct client{
        int sd;
        char addr[INET_ADDRSTRLEN];
        unsigned short port;
        struct client *next;
};

struct client *insert(struct client *clients, int sd, char addr[INET_ADDRSTRLEN], unsigned short port);
struct client *delete(struct client *clients, int sd);

void server( unsigned char * key);

#endif
