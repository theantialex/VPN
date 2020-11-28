#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>
#include <stdlib.h>

typedef struct {
	char* name;
    char* password;
} client_id;

typedef struct {
    client_id identification;
    int sock;
} client_t;

client_t* client_create(int socket, client_id id);
void send_data(client_t* client);
void get_response(client_t* client);

#endif