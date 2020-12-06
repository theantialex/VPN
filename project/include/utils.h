#ifndef PROJECT_INCLUDE_UTILS_H_
#define PROJECT_INCLUDE_UTILS_H_

#define FAILURE -1
#define SUCCESS 0

#include <stdint.h>
#include <stdio.h>

typedef struct {
	char *root;
	char *address;
	uint16_t port;
	int backlog;
	// char *pidfile;
} hserver_config_t;

typedef struct {
	char* name;
    char* password;
} client_id;

int send_all(int sock, char* str, int len);
int recv_all(int sock, int len , char* result_str);

void free_client_id (client_id* id);

#endif  // PROJECT_INCLUDE_UTILS_H_
