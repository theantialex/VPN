#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "client.h"
#include <stdio.h>


int main(int argc, char *argv[]) {

    if (argc < 2) {
        return -1;
    }

    client_id id = {argv[1], argv[2]};
    int clt_socket;
    clt_socket  = socket(AF_INET , SOCK_STREAM, 0);
    client_t* client = client_create(clt_socket, id);
    
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(80);
    inet_aton("127.0.0.1", &dest_addr.sin_addr);
    
    if (connect(client->sock, (struct sockaddr *) &dest_addr, sizeof(dest_addr) == 0)) {
        puts("client connected");
        send_data(client);
    }

    return 0;
}