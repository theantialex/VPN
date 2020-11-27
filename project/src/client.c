#include "client.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

client_t* client_create(int socket, client_id id) {
    client_t* client = calloc(1, sizeof(client));
    client->identification = id;
    client->sock = socket;
    return client;
}

void send_data(client_t* client) {
    puts("data ");
    printf("%ld", send(client->sock, (void*) &client->identification, sizeof(client->identification), 0));
    printf("Oh dear, something went wrong with read()! %s\n", strerror(errno));
    puts("data sent");
}