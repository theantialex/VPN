#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/select.h>

#include "client.h"
#include "tun.h"
#include "utils.h"

#define SERVER_ADDRESS "127.0.0.1"
#define ACCESS_DENIED "Access denied"
#define TUN_NAME "tun0"
#define SERVER_PORT 80
#define BUFSIZE 2000


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

int connect_to_server(int sock) {
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(5384);
    if (inet_aton(SERVER_ADDRESS, &dest_addr.sin_addr) == 0) {
        goto error;
    }

    if (connect(sock, (struct sockaddr *) &dest_addr, sizeof(dest_addr)) == -1) {
        goto error;
    }
    puts("Client connected");

error:
    printf("Error occured while running client: %s\n", strerror(errno));
	return FAILURE;
}

int data_anticipation(int tun_socket, int client_server_socket) {
//   int max_sock = (tun_socket > client_server_socket)? tun_socket:client_server_socket;

//   while (true) {
//     int available_fds;
//     fd_set rd_set;
//     int post_socket;
//     socklen_t remotelen;
//     struct sockaddr_in remote;

//      /* wait for connection request */

//     remotelen = sizeof(remote);
//     memset(&remote, 0, remotelen);
//     if (post_socket = accept(tun_socket, (struct sockaddr*)&remote, &remotelen) < 0){
//       perror("accept()");
//       exit(1);
//     }

//     FD_ZERO(&rd_set);
//     FD_SET(tun_socket, &rd_set); 
//     FD_SET(client_socket, &rd_set);

//     available_fds = select(max_sock + 1, &rd_set, NULL, NULL, NULL);

//     if (available_fds < 0 && errno == EINTR){
//       continue;
//     }

//     if (available_fds < 0) {
//       perror("select()");
//       exit(1);
//     }

//     if (FD_ISSET(tun_socket, &rd_set)){
//       /* data from tun: just read it and write it to the network */
    
//       char buffer[BUFSIZE];

//       // TODO: ROUTING
//       recv_all(tun_socket, BUFSIZE, buffer);

//       /* write length + packet */
//       int length = htons(strlen(buffer));
//       if (send(serv_socket, &length, sizeof(length), 0) == -1) {
//           goto error;
//       }
//       if (send_all(serv_socket, buffer, length) == -1) {
//           goto error;
//       }

//     }

//     if (FD_ISSET(serv_socket, &rd_set)){
//       /* data from the network: read it, and write it to the tun interface. 
//        * We need to read the length first, and then the packet */

//       /* Read length */    
//       int data_len;
//       if (recv(serv_socket, &data_len, sizeof(data_len), 0) == -1) {
//           goto error;
//       }
//       char* data = (char*) calloc(data_len, sizeof(char));
//       if (recv_all(serv_socket, data_len, data) == -1) {
//           goto error;
//       }

//       /* write it into the tun/tap interface */ 
//       if (send_all(tun_socket, data, data_len) == -1) {
//           goto error;
//       }
//     }

  //  }
error:
    printf("Error occured while running data anticipation: %s\n", strerror(errno));
	return FAILURE;
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

    connect_to_server(client->sock);

    if (send_data(client) == -1) {
        goto error;
    }
    char* get_addr = get_response(client);
    if (get_addr == NULL || get_addr == ACCESS_DENIED) {
        goto error;
    }

    int tun_socket = create_client_tun(TUN_NAME, SERVER_PORT, SERVER_ADDRESS);
    if (data_anticipation(tun_socket, client->sock) != 0) {
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
