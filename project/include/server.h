#ifndef _PROJECT_INCLUDE_SERVER_H_
#define _PROJECT_INCLUDE_SERVER_H_
#include <stdint.h>
#include <stdio.h>

#include "client.h"

typedef struct {
	char *root;
	char *address;
	uint16_t port;
	int backlog;
	// char *pidfile;
} hserver_config_t;

typedef struct {
	int sock;
} server_t;


server_t* server_create(hserver_config_t *config);
void server_close(server_t *server);
int server_run(hserver_config_t *config);
int process_setup_signals();

client_id* get_client_id(int socket);
int send_access_to_client(client_id* id, int socket);

int client_identify(client_id* id, char* network_addr);
int add_active_client(char* client_name, char* client_network);

#endif  // _PROJECT_INCLUDE_SERVER_H_
