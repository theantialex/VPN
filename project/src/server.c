#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "server.h"
#include "utils.h"

#define active_clients_fpath "./project/data/active_clients.txt"
#define config_fpath "./project/data/config.txt"

volatile int process_exited = 0;

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
		free(id->name);
		free(id->password);
		free(id);
		goto error;
	}

	return id;

error:
	printf("Error occured while getting client id: %s\n", strerror(errno));
	return NULL;
}

int send_access_to_client(client_id* id, int socket) {
	char* network_addr = (char*) calloc(1024, sizeof(char));
	if (network_addr == NULL) {
		goto error;
	}

	if (client_identify(id, network_addr) == -1) {
		strcpy(network_addr, "Access denied");
	} else {
		add_active_client(id->name, network_addr);
	}

	size_t network_addr_len = strlen(network_addr);

	if (send(socket, &network_addr_len, sizeof(network_addr_len), 0) == -1) {
		free(network_addr);
		goto error;
	}	

	if (send_all(socket, network_addr, strlen(network_addr)) == -1) {
		free(network_addr);
		goto error;
	}
	
	free(network_addr);

	return SUCCESS;

error:
	printf("Error occured while sending access to client: %s\n", strerror(errno));
	return FAILURE;
}

int server_run(hserver_config_t *config) {
    server_t* server = server_create(config);
	if (server == NULL) {
		goto error;
	}

    while (true) {
		struct sockaddr_in client;
		socklen_t len = sizeof(client);
		int socket = accept(server->sock, (struct sockaddr *)&client, &len);

		char s[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET, &(((struct sockaddr_in*) &client)->sin_addr), s, sizeof(s));
		printf("Got a connection from %s\n", s);

		if (socket == -1) {
			free(server);
			goto error;
		}
		
		client_id* id = get_client_id(socket);
		if (id == NULL) {
			server_close(server);
			goto error;
		}

		printf("Got data: %s %s\n", id->name, id->password);

		if (send_access_to_client(id, socket) == -1) {
			free(id->name);
			free(id->password);
			free(id);

			server_close(server);
			goto error;
		};		

		free(id->name);
		free(id->password);
		free(id);

		if (process_exited)
			break;

		/*if (!client)
			continue;*/
    }
    server_close(server);
    return SUCCESS;

error:
	return FAILURE;
}

void server_close(server_t *server) {
    close(server->sock);
	free(server);
}
