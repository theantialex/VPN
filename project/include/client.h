#ifndef PROJECT_INCLUDE_CLIENT_H_
#define PROJECT_INCLUDE_CLIENT_H_

#include <stdlib.h>
#include <sys/socket.h>

#include "utils.h"

typedef struct {
    network_id_t net_id;
    int sock;
} client_t;

typedef struct {
    char* network_addr;
    int mask;
} network_addr_t;

struct recv_param_s {
    int socket;
};


client_t* client_create(int socket, network_id_t net_id);
// int send_id(client_t* client);
char* get_response(client_t* client);
// int routing(network_addr_t* web_addr, char* tun_name);

void read_tun_event_handler(int tun_socket, short flags, struct recv_param_s* recv_tun_param);
void recv__clt_event_handler(int client_server_socket, short flags, struct recv_param_s* recv_param);
int event_anticipation(int tun_socket, int client_server_socket);

int connect_to_server(int sock, char* server_addr);

// int client_run(client_id* id, char* server_addr);
int client_save_data(network_id_t net_id, char* server_addr);
int send_cmd_request(char* cmd, client_t* client);
int get_server_addr(network_id_t* net_id, char server_addr[MAX_STORAGE]);
int network_creation_response(char* response);
int set_connection_process(char* get_addr, int socket);
char* get_cmd_response(int socket);
int cmd_choice(char* cmd, char* response, int socket);
int client_run_cmd(char* cmd, network_id_t net_id, char* param[]);

#endif  // PROJECT_INCLUDE_CLIENT_H_