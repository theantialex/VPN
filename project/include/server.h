#ifndef PROJECT_INCLUDE_SERVER_H_
#define PROJECT_INCLUDE_SERVER_H_

#include <event2/event.h>
#include <event2/event-config.h>

#include "utils.h"

typedef struct {
	int sock;
	struct event_base *base;
} server_t;

typedef struct {
	int network_id;
	int client_id;
} storage_id_t;

typedef struct {
	int client_socket;
	int tun_socket;
} client_in_t;

typedef struct {
	struct event* clt_recv_event;
	struct event* tun_recv_event;
} client_event_t;

struct clt_recv_param_s {
	int server_tun_socket;
};

struct tun_recv_param_s {
	int client_socket;
};

struct server_accept_param_s {
	int client_server_socket;
	int cmd_id;
	char *response;
	storage_id_t *clt_db_id;
	struct event_base *base;
};


server_t* server_create(hserver_config_t *config);
void server_close(server_t *server);
int server_run(hserver_config_t *config);
int process_setup_signals();

client_id* get_client_id(int socket);

int get_first_available_client(int network_id);

int get_client_access(client_id* id, int network_storage_id, char* access_result);
int send_access_to_client(client_id* id, int socket, int network_storage_id, int* client_storage_id);

int client_identification_process(int client_sock, int server_sock, hserver_config_t* server_param, storage_id_t* clt_storage_id);

int client_identify(network_id_t* id, char* network_addr);
int add_active_client(char* client_name, char* client_network);

void free_client_db();

#endif  // PROJECT_INCLUDE_SERVER_H_
