#ifndef SERVER_H
#define SERVER_H
#include <stdint.h>
#include <stdlib.h>
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
char* recv_all(int sock, int len);

#endif