#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#include "server.h"

struct client *insert(struct client *clients, int sd, char addr[INET_ADDRSTRLEN], unsigned short port) {

	struct client *tmp=NULL;

	tmp = (struct client *)malloc(sizeof(struct client));
	tmp->sd = sd;
	memcpy(tmp->addr, addr, INET_ADDRSTRLEN);
	tmp->port = port;
	tmp->next = NULL;

	tmp->next = clients;
	clients = tmp;

	return clients;
}

struct client *delete(struct client *clients, int sd) {

	struct client *curr = clients, *prev = NULL;

	if (curr == NULL) {
		fprintf(stderr, "Internal error! Tried to delete from empty list.\n");
		exit(1);
	}

	else {
		while (curr->sd != sd) {
			prev = curr;
			curr = curr->next;
		}
		if (prev == NULL)
			clients = curr->next;
		else {
			prev->next = curr->next;
		}
		free(curr);
	}
	return  clients;
}
