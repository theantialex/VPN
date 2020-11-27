#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include "server.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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
	if (sock == -1)
		return NULL;

	int enable = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
		goto error;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(config->port);
	
	if (inet_aton(config->address, &addr.sin_addr) == -1)
		goto error; 
	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		goto error;
	}

	if (listen(sock, config->backlog) == -1)
		goto error;

    server_t *server = calloc(1, sizeof(server_t));
	if (!server) {
		close(sock);
		return NULL;
	}

	server->sock = sock;
	
	return server;

error:
	close(sock);
	return NULL;
}

int process_setup_signals() {
	struct sigaction action = {0};
	action.sa_handler = sigterm_handler;
	if (sigaction(SIGTERM, &action, NULL) == -1)
		return -1;
	return 0;
}

int server_run(hserver_config_t *config) {
	//printf("no1\n");
    server_t* server = server_create(config);
	 printf("no2\n");
    while (true) {
		printf("before\n");
		int socket = accept(server->sock, NULL, NULL);
		printf("after\n");
		if (accept(server->sock, NULL, NULL) == -1) 
			return -1;
		printf("no\n");
		client_id id;
		recv(socket, (void*) &id, sizeof(client_id), 0);
		printf("%s %s", id.password, id.name);

		if (process_exited)
			break;

		/*if (!client)
			continue;*/
    }
    server_close(server);
    return 0;
}

void server_close(server_t *server) {
    close(server->sock);
	free(server);
}
