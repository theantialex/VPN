#include "client.h"
#include <stdio.h>
#include <string.h>

client_t* client_create(int socket, client_id id) {
    client_t* client = calloc(1, sizeof(client));
    client->identification = id;
    client->sock = socket;
    return client;
}

void send_data(client_t* client) {
    // char name[255];
    // strcpy(name, client->identification.name);
    // char pass[255];
     //strcpy(pass, client->identification.password);
    int len = strlen(client->identification.name);
    char name[255];
    strcpy(name, client->identification.name);
    send(client->sock, &len, sizeof(len), 0);
    send(client->sock, &name, sizeof(name), 0);

    len = strlen(client->identification.password);
    printf("length: %d\n", len);
    char pass[255];
    strcpy(pass, client->identification.password);
    send(client->sock, &len, sizeof(len), 0);
    send(client->sock, &pass, sizeof(pass), 0);

    // send(client->sock, &client->identification.name, sizeof(client->identification.name), 0);
    // send(client->sock, &len, sizeof(len), 0);
    // send(client->sock, &client->identification.password, sizeof(client->identification.password), 0);

    //int bytes = send(client->sock, &name, sizeof(name), 0);
    // bytes += send(client->sock, &pass, sizeof(pass), 0);

    // if (bytes ==  sizeof(client->identification)) {
    //     puts("All data sent");
    // }
}