#ifndef PROJECT_INCLUDE_UTILS_H_
#define PROJECT_INCLUDE_UTILS_H_

#define FAILURE -1
#define SUCCESS 0
#define MAX_STORAGE 250
#define CREATE_CMD "create"
#define CONNECT_CMD "connect"
#define DISCONNECT_CMD "leave"
#define DELETE_CMD "delete"

#define CREATE_SUCCESS "Network succesfully allocated"
#define CREATE_FAILURE "Can't allocate network"
#define CREATE_SAME_ERROR "Network with this name already exists"

#define DELETE_SUCCESS "Network succesfully deleted"
#define DELETE_FAILURE "Can't delete network"

#define DISCONNECT_SUCCESS "Client succesfully disconnected"

#define EMPTY_RESPONSE "Can't process cmd"
#define NOT_EXIST_RESPONSE "Can't process cmd. Network or client doesn't exist"
#define NW_NOT_EXIST_RESPONSE "Can't process cmd. Network doesn't exist"


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

typedef struct {
	char* name;
	char* password;
} network_id_t;

int send_all(int sock, char* str, int len);
int recv_all(int sock, int len , char* result_str);

void free_client_id (client_id* id);

#endif  // PROJECT_INCLUDE_UTILS_H_
