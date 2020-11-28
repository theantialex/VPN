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

char* recv_all(int sock, int len) {
	printf("len: %d\n", len);
	char* str = (char*) calloc(len, sizeof(char));
	char c;
	int i;
	for (i = 0; i < len; ++i) {
		if (recv(sock, &c, 1, 0) == -1) {
			goto error;
		}
		str[i] = c;
		printf("%c\n", c);
	}
	str[i] = '\0';
	return str;

error:
	close(sock);
	return NULL;
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

	puts("Server created");

	if (inet_aton(config->address, &addr.sin_addr) == -1)
		goto error; 
	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		goto error;
	}

	if (listen(sock, config->backlog) == -1)
		goto error;

	puts("Listening...");

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
    server_t* server = server_create(config);

    while (true) {

		struct sockaddr_in client;
		socklen_t len = sizeof(client);
		int socket = accept(server->sock, (struct sockaddr *)&client, &len);

		char s[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET, &(((struct sockaddr_in*) &client)->sin_addr), s, sizeof(s));
		printf("Got a connection from %s\n", s);

		if (socket == -1)
			return -1;
		
		client_id id;

		int id_len;
		if (recv(socket, &id_len, sizeof(id_len), 0) == -1) {
			return -1;
		}
		id.name = recv_all(socket, id_len);

		recv(socket, &id_len, sizeof(id_len), 0);
		
		if (recv(socket, &id_len, sizeof(id_len), 0) > 0 && id_len != 0) 
			return -1;
		id.password = recv_all(socket, id_len);

		printf("%s %s\n", id.name, id.password);
		// client_id id;
		// id.name = (char*) calloc(1000, sizeof(char));
		// id.password = (char*) calloc(1000, sizeof(char));
		// char name[255];
		// char pass[255];
		// recv(socket, &name, 255, 0);
		// recv(socket, &pass, 255, 0);
		// printf("recv code: %d %d\n", size, res2);	
		// printf("%s %s\n", pass, name);
		// printf("Error: %s\n", strerror(errno));

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
