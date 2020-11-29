#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>

#include "client.h"
#include "utils.h"

client_t* client_create(int socket, client_id id) {
    client_t* client = calloc(1, sizeof(*client));
    if (client == NULL) {
        return NULL;
    }

    client->identification = id;
    client->sock = socket;
    return client;
}

int send_data(client_t* client) {
    int len = strlen(client->identification.name);
    if (send(client->sock, &len, sizeof(len), 0) == -1) {
        goto error;
    }

    if (send_all(client->sock, client->identification.name, strlen(client->identification.name)) == -1) {
        goto error;
    }

    len = strlen(client->identification.password);
    if (send(client->sock, &len, sizeof(len), 0) == -1) {
        goto error;
    }

    if (send_all(client->sock, client->identification.password, strlen(client->identification.password)) == -1) {
        goto error;
    }

    puts("Data sent");
    return SUCCESS;

error:
    printf("Error occured while sending data from client: %s\n", strerror(errno));
	return FAILURE;
}

char* get_response(client_t* client) {
    size_t response_len;
    if (recv(client->sock, &response_len, sizeof(response_len), 0) == -1) {
        goto error;
    }

    char* get_addr = (char*) calloc(response_len, sizeof(char));
    if (get_addr == NULL) {
        goto error;
    }

    if (recv_all(client->sock, response_len, get_addr) == -1) {
        free(get_addr);
        goto error;
    }

    printf("Responce received: %s\n", get_addr);
    return get_addr;

error:
    printf("Error occured while getting response at client: %s\n", strerror(errno));
	return NULL;
}

int client_run(client_id* id) {
    int clt_socket;
    clt_socket  = socket(AF_INET, SOCK_STREAM, 0);
    if (clt_socket == -1) {
        printf("Error occured while running client: %s\n", strerror(errno));
        return FAILURE;
    }

    client_t* client = client_create(clt_socket, *id);
    if (client == NULL) {
        printf("Error occured while running client: %s\n", strerror(errno));
        goto error;
    }

    puts("Client created");

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(5384);
    if (inet_aton("127.0.0.1", &dest_addr.sin_addr) == 0) {
        goto error;
    }

    if (connect(client->sock, (struct sockaddr *) &dest_addr, sizeof(dest_addr)) == -1) {
        goto error;
    }

    puts("Client connected");

    if (send_data(client) == -1) {
        goto error;
    }

    char* get_addr = get_response(client);
    if (get_addr == NULL) {
        goto error;
    }

    free(get_addr);
    free(client);

    return SUCCESS;

error:
    printf("Error occured while running client: %s\n", strerror(errno));
    free(client);
	return FAILURE;
}
