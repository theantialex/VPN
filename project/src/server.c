#include <arpa/inet.h>
#include <errno.h>
#include <event2/event.h>
#include <event2/event-config.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "server.h"
#include "tun.h"

#define active_clients_fpath "./project/data/active_clients.txt"
#define config_fpath "./project/data/config.txt"
#define MAX_STORAGE 250
#define BUFSIZE 2000

volatile int process_exited = 0;

static client_in_t* client_db[MAX_STORAGE][MAX_STORAGE] = {};

int available_client_in_nw[MAX_STORAGE][MAX_STORAGE] = {};
int available_network[MAX_STORAGE] = {};

static void sigterm_handler(int signum) {
	if (signum) {
		process_exited = 1;
	} else {
		process_exited = 1;
	}
}

server_t* server_create(hserver_config_t *config) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		printf("Error occured: %s\n", strerror(errno));
		return NULL;
	}

	int enable = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
		goto error;
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(config->port);

	puts("Server created");

	if (inet_aton(config->address, &addr.sin_addr) == -1) {
		goto error; 
	}

	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		goto error;
	}

	if (listen(sock, config->backlog) == -1)
		goto error;

	puts("Listening...");

    server_t *server = calloc(1, sizeof(server_t));
	if (server == NULL) {
		goto error;
	}

	server->sock = sock;

	FILE* active_clients_file = fopen(active_clients_fpath, "w");
	if (active_clients_file == NULL) {
		free(server);
		goto error;
	}
	fclose(active_clients_file);
	
	return server;

error:
	printf("Error occured while creating server: %s\n", strerror(errno));
	close(sock);
	return NULL;
}

int process_setup_signals() {
	struct sigaction action = {0};
	action.sa_handler = sigterm_handler;
	if (sigaction(SIGTERM, &action, NULL) == -1) {
		goto error;
	}

	return SUCCESS;

error:
	printf("Error occured while setting up signals: %s\n", strerror(errno));
	return FAILURE;
}

int client_identify(client_id* id, char* network_addr) {
	FILE* config_file = fopen(config_fpath, "r");
	if (config_file == NULL) {
		return FAILURE;
	}

	char* c_name = (char*)calloc(1024, sizeof(char));
	if (c_name == NULL) {
		goto error;
	}

	char* c_password = (char*)calloc(1024, sizeof(char));
	if (c_password == NULL) {
		free(c_name);
		goto error;
	}

	char* c_network = (char*)calloc(1024, sizeof(char));
	if (c_network == NULL) {
		free(c_name);
		free(c_network);
		goto error;
	}

	while(fscanf(config_file, "%s %s %s", c_name, c_password, c_network) == 3) {
		if (strlen(c_name) == strlen(id->name) && strncmp(c_name, id->name, strlen(c_name)) == 0) {
			if (strlen(c_password) == strlen(id->password) && strncmp(c_password, id->password, strlen(c_password)) == 0) {
				strncpy(network_addr, c_network, strlen(c_network));

				fclose(config_file);

				free(c_name);
				free(c_password);
				free(c_network);

				return SUCCESS;
			}
		}
	}

	fclose(config_file);

	free(c_name);
	free(c_password);
	free(c_network);

	return FAILURE;

error:
	fclose(config_file);
	printf("Error occured while identifying client: %s\n", strerror(errno));
	return FAILURE;
}

int add_active_client(char* client_name, char* client_network) {
	FILE* aclient_file = fopen(active_clients_fpath, "a+");
	if (aclient_file == NULL) {
		goto error;
	}

	if (fprintf(aclient_file, "%s %s\n", client_name, client_network) < 0) {
		fclose(aclient_file);
		goto error;
	}

	fclose(aclient_file);
	return SUCCESS;

error:
	printf("Error occured while adding active client: %s\n", strerror(errno));
	return FAILURE;
}

client_id* get_client_id(int socket) {	
	int id_len;
	if (recv(socket, &id_len, sizeof(id_len), 0) == -1) {
		goto error;
	}

	client_id* id = calloc(1, sizeof(client_id));
	if (id == NULL) {
		goto error;
	}

	id->name = (char*) calloc(id_len, sizeof(char));
	if (id->name == NULL) {
		free(id);
		goto error;
	}

	if (recv_all(socket, id_len, id->name) == -1) {
		free(id->name);
		free(id);
		goto error;
	}
	
	if (recv(socket, &id_len, sizeof(id_len), 0) == -1) {
		free(id->name);
		free(id);
		goto error;
	}

	id->password = (char*) calloc(id_len,sizeof(char));
	if (id->password == NULL) {
		free(id->name);
		free(id);
		goto error;
	}

	if (recv_all(socket, id_len, id->password) == -1) {
		free_client_id(id);
		free(id);
		goto error;
	}

	return id;

error:
	printf("Error occured while getting client id: %s\n", strerror(errno));
	return NULL;
}

int get_client_access(client_id* id, int network_storage_id, char* access_result) {
	if (client_identify(id, access_result) == -1) {
		strcpy(access_result, "Access denied");

		return -1;
	}

	int client_storage_id = get_first_availiable_client(network_storage_id);

	if (client_storage_id == -1) {
		strcpy(access_result, "Access denied");
		puts("The network is full. No access given");
		return -1;
	}

	add_active_client(id->name, access_result);

	return client_storage_id;
}

int send_access_to_client(client_id* id, int socket, int network_storage_id, int* client_storage_id) {
	char access_result[MAX_STORAGE];

	*client_storage_id = get_client_access(id, network_storage_id, access_result);

	size_t access_res_len = strlen(access_result);

	if (send(socket, &access_res_len, sizeof(access_res_len), 0) == -1) {
		goto error;
	}	

	if (send_all(socket, access_result, access_res_len) == -1) {
		goto error;
	}

	return SUCCESS;

error:
	printf("Error occured while sending access to client: %s\n", strerror(errno));
	return FAILURE;
}

int get_first_availiable_client(int network_id) {
	for (int i = 0; i < MAX_STORAGE; i++) {
		if (available_client_in_nw[network_id][i] == 0) {
			return i;
		}
	}
	return -1;
}

int client_identification_process(int client_sock, int server_sock, hserver_config_t* server_param, storage_id_t* clt_db_id) {
	client_id* id = get_client_id(client_sock);
	if (id == NULL) {
		goto error;
	}

	printf("Got client id: %s %s\n", id->name, id->password);

	int client_storage_id;
	if (send_access_to_client(id, client_sock, clt_db_id->network_id, &client_storage_id) == -1) {
		free_client_id(id);
		free(id);

		goto error;
	};

	if (client_storage_id != -1) {
		char* tun_name = "server_tun";
		char tun_id_str[10] = {};
		int post_sock;
		sprintf(tun_id_str, "%d", client_storage_id);

		char res_name[20];
		snprintf(res_name, 20, "%s%s", tun_name, tun_id_str);
		
		char access_res[MAX_STORAGE];
		get_client_access(id, clt_db_id->network_id, access_res);

		int tun_origin_sock = create_server_tun(res_name, server_param, access_res);
		if (tun_origin_sock == -1) {
			goto error;
		}

		post_sock = accept(server_sock, NULL, NULL);

		client_in_t* client_in = calloc(1, sizeof(client_in_t));
		client_in->client_socket = client_sock;

		client_in->tun_socket = post_sock;

		clt_db_id->client_id = client_storage_id;

		client_db[clt_db_id->network_id][clt_db_id->client_id] = client_in;
		available_client_in_nw[clt_db_id->network_id][client_storage_id] = 1;

	}

	free_client_id(id);
	free(id);

	return SUCCESS;

error:
	printf("Error occured while running client identification process: %s\n", strerror(errno));
	return FAILURE;
}

int accept_event_handler(int server_sock, hserver_config_t *config, storage_id_t* storage_id) {
	int client_server_socket;

	// fcntl(server_sock, F_SETFL, O_NONBLOCK);

	struct sockaddr_in client;
	socklen_t len = sizeof(client);
	if ((client_server_socket = accept(server_sock, (struct sockaddr *)&client, &len)) <= 0){
		return FAILURE;
    }

	char s[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET, &(((struct sockaddr_in*) &client)->sin_addr), s, sizeof(s));
	printf("Got a connection from %s\n", s);

	if (client_server_socket == -1) {
		goto error;
	}

	if (client_identification_process(client_server_socket, server_sock, config, storage_id) == FAILURE) {
		goto error;
	}
	return SUCCESS;

error:
    printf("Error occured while accepting event handler: %s\n", strerror(errno));
	return FAILURE;
}

void client_recv_event_handler(int client_server_socket, short flags, struct clt_recv_param_s* params) {
	puts("Got something in clt_recv event handler!");

    if (flags == 0) {
        perror("flags");
        exit(1);
    }

    char buffer[BUFSIZE];

    if (recv_all(client_server_socket, BUFSIZE, buffer) == -1) {
        perror("recv_all()");
        exit(1);
    }
	printf("Received %zu of data\n", strlen(buffer));

    if (send_all(params->server_tun_socket, buffer, strlen(buffer)) == -1) {
        perror("send_all()");
        exit(1);
    }
}

void tun_recv_event_handler(int tun_socket, short flags, struct tun_recv_param_s* params) {
	puts("Got something in tun_recv event handler!");

    if (flags == 0) {
        perror("flags");
        exit(1);
    }

    char buffer[BUFSIZE];

    if (recv_all(tun_socket, BUFSIZE, buffer) == -1) {
        perror("recv_all()");
        exit(1);
    }

    if (send_all(params->client_socket, buffer, strlen(buffer)) == -1) {
        perror("send_all()");
        exit(1);
    }
}

int event_anticipation(server_t* server, hserver_config_t *config, storage_id_t* clt_db_id) {
	// struct event_base* ev_base_arr[250];
	// int i = 0;
	// int j = 0;

	if (accept_event_handler(server->sock, config, clt_db_id) == SUCCESS) {

		int client_server_sock = client_db[clt_db_id->network_id][clt_db_id->client_id]->client_socket;
		int tunnel_sock = client_db[clt_db_id->network_id][clt_db_id->client_id]->tun_socket;

		struct event_base* base = event_base_new();
		if (!base) {
			perror("base");
			exit(1);
		}

		struct clt_recv_param_s clt_param = { tunnel_sock };

		struct event* clt_recv_event = event_new(base, client_server_sock,  EV_READ | EV_PERSIST, 
		(event_callback_fn)client_recv_event_handler, (void*)&clt_param);
		if (!clt_recv_event) {
			goto error;
		}
		if (event_add(clt_recv_event, NULL) < 0) {
			goto error;
		}

		struct tun_recv_param_s tun_param = { client_server_sock };
		struct event* tun_recv_event = event_new(base, tunnel_sock, EV_READ | EV_PERSIST, 
		(event_callback_fn)tun_recv_event_handler, (void*)&tun_param);
		if (!tun_recv_event) {
			goto error;
		}
		if (event_add(tun_recv_event, NULL) < 0) {
			goto error;
		}

		if (event_base_dispatch(base) < 0) {
			goto error;
		}

		event_base_free(base);
	} else {
		goto error;
	}
	// while(true) {
	// 	if (accept_event_handler(server->sock, config, clt_db_id) == SUCCESS) {

	// 		int client_server_sock = client_db[clt_db_id->network_id][clt_db_id->client_id]->client_socket;
	// 		int tunnel_sock = client_db[clt_db_id->network_id][clt_db_id->client_id]->tun_socket;

	// 		struct event_base* base = event_base_new();
	// 		if (!base) {
    //             perror("base");
    //             exit(1);
    //         }

	// 		struct clt_recv_param_s clt_param = { tunnel_sock };

	// 		struct event* clt_recv_event = event_new(base, client_server_sock,  EV_READ | EV_PERSIST, 
	// 		(event_callback_fn)client_recv_event_handler, (void*)&clt_param);
	// 		if (!clt_recv_event) {
    //             goto error;
    //         }
	// 		if (event_add(clt_recv_event, NULL) < 0) {
    //             goto error;
    //         }

	// 		struct tun_recv_param_s tun_param = { client_server_sock };
	// 		struct event* tun_recv_event = event_new(base, tunnel_sock, EV_READ | EV_PERSIST, 
	// 		(event_callback_fn)tun_recv_event_handler, (void*)&tun_param);
	// 		if (!tun_recv_event) {
    //             goto error;
    //         }
	// 		if (event_add(tun_recv_event, NULL) < 0) {
    //             goto error;
    //         }

	// 		ev_base_arr[i] = base;
	// 		i++;
	// 	}
	// 	j = 0;
	// 	while(j < i) {
	// 		if (ev_base_arr[j] != NULL) {
	// 			event_base_loop(ev_base_arr[j], EVLOOP_NONBLOCK | EVLOOP_ONCE);
	// 		}
	// 		j++;
	// 	}
	// }

    return SUCCESS;

error:
    printf("Error occured while running event anticipation: %s\n", strerror(errno));
	return FAILURE;

}

int server_run(hserver_config_t *config) {
   server_t* server = server_create(config);
	if (server == NULL) {
		goto error;
	}

	// Get actual network id in later realisation
	storage_id_t client_db_id = {0,-1};

	if (event_anticipation(server, config, &client_db_id) == FAILURE) {
		goto error;
	}

    server_close(server);
    return SUCCESS;

error:
	return FAILURE;
}

void server_close(server_t *server) {
    close(server->sock);
	free(server);
	free_client_db();
}

void free_client_db() {
	for (int i = 0; i < MAX_STORAGE; i++) {
		for (int j = 0; j < MAX_STORAGE; j++) {
			if (client_db[i][j] != NULL) {
				free(client_db[i][j]);
			}
		}
	}
}
