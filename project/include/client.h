#ifndef PROJECT_INCLUDE_CLIENT_H_
#define PROJECT_INCLUDE_CLIENT_H_

#include <event2/event.h>
#include <event2/event-config.h>

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

typedef struct {
	struct event* clt_recv_event;
	struct event* tun_recv_event;
} client_event_t;


client_t* client_create(int socket, network_id_t net_id);
char* get_response(client_t* client);

void read_tun_event_handler(int tun_socket, short flags, struct recv_param_s* recv_tun_param);
void recv__clt_event_handler(int client_server_socket, short flags, struct recv_param_s* recv_param);
int event_anticipation(int tun_socket, int client_server_socket);

int connect_to_server(int sock, char* server_addr);

int client_save_data(network_id_t net_id, char* server_addr);
int send_cmd_request(char* cmd, client_t* client);
int network_creation_response(char* response);
int set_connection_process(char* get_addr, int socket);
int get_cmd_response(int socket, char* response);
int cmd_choice(char* cmd, char* response, int socket);
int client_run_cmd(char* cmd, network_id_t net_id, char* param[]);

#endif  // PROJECT_INCLUDE_CLIENT_H_