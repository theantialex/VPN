#include <stdio.h>
#include <string.h>

#include "client.h"
#include "utils.h"

client_t* client_create(int socket, client_id id) {
    client_t* client = calloc(1, sizeof(client));
    client->identification = id;
    client->sock = socket;
    return client;
}

void send_data(client_t* client) {
    int len = strlen(client->identification.name);
    send(client->sock, &len, sizeof(len), 0);
    send_all(client->sock, client->identification.name, strlen(client->identification.name));

    len = strlen(client->identification.password);
    send(client->sock, &len, sizeof(len), 0);
    if (send_all(client->sock, client->identification.password, strlen(client->identification.password)) == 0) {
        puts("Data sent");
    }
}

void get_response(client_t* client) {
    size_t response_len;
    recv(client->sock, &response_len, sizeof(response_len), 0);
    printf("get len %ld", response_len);
    char* get_addr = (char*) calloc(response_len, sizeof(char));
    recv_all(client->sock, response_len, get_addr);
    printf("Responce received: %s", get_addr);
}
