#ifndef PROJECT_INCLUDE_CLIENT_H_
#define PROJECT_INCLUDE_CLIENT_H_

#include <stdlib.h>
#include <sys/socket.h>


typedef struct {
	char* name;
    char* password;
} client_id;

typedef struct {
    client_id identification;
    int sock;
} client_t;

client_t* client_create(int socket, client_id id);
int send_id(client_t* client);
char* get_response(client_t* client);

int client_run(client_id* id);
// int data_anticipation(int tun_socket, int client_server_socket);

#endif  // PROJECT_INCLUDE_CLIENT_H_