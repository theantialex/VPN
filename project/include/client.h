#ifndef PROJECT_INCLUDE_CLIENT_H_
#define PROJECT_INCLUDE_CLIENT_H_

#include <stdlib.h>
#include <sys/socket.h>

#include "utils.h"

typedef struct {
    client_id identification;
    int sock;
} client_t;

typedef struct {
    char* network_addr;
    int mask;
} network_addr_t;

struct accept_param_s {
    int client_server_sock;
    network_addr_t* web_addr;
};

struct recv_param_s {
    int tun_socket;
};

client_t* client_create(int socket, client_id id);
int send_id(client_t* client);
char* get_response(client_t* client);

void accept_event_handler(int tun_socket, short flags, struct accept_param_s* accept_param);
void recv_event_handler(int client_server_socket, short flags, struct recv_param_s* recv_param);
int event_anticipation(int tun_socket, int client_server_socket, network_addr_t* web_addr);

int client_run(client_id* id);

#endif  // PROJECT_INCLUDE_CLIENT_H_