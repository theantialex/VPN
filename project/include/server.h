#ifndef PROJECT_INCLUDE_SERVER_H_
#define PROJECT_INCLUDE_SERVER_H_

#include "utils.h"

typedef struct {
	int sock;
} server_t;

typedef struct {
	int network_id;
	int client_id;
} storage_id_t;

typedef struct {
	int client_socket;
	int tun_socket;
} client_in_t;

struct accept_param_s {
	struct sockaddr_in address;
	socklen_t length;
};

struct clt_recv_param_s {
	int server_tun_socket;
};

struct tun_recv_param_s {
	int client_socket;
};


server_t* server_create(hserver_config_t *config);
void server_close(server_t *server);
int server_run(hserver_config_t *config);
int process_setup_signals();

int get_first_availiable_client(int network_id);

int get_client_access(client_id* id, int network_storage_id, char* access_result);
int send_access_to_client(client_id* id, int socket, int network_storage_id, int* client_storage_id);
int add_active_client(char* client_name, char* client_network);

int client_identification_process(int client_sock, hserver_config_t* server_param, storage_id_t* clt_storage_id);
client_id* get_client_id(int socket);
int client_identify(client_id* id, char* network_addr);

int event_anticipation(server_t* server, hserver_config_t *config, storage_id_t* clt_db_id);
void client_recv_event_handler(int client_server_socket, short flags, struct clt_recv_param_s params);
int accept_event_handler(int server_sock, hserver_config_t *config, storage_id_t* storage_id);

void free_client_db();

#endif  // PROJECT_INCLUDE_SERVER_H_
