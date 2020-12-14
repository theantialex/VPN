#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>

#include <event2/event.h>
#include <event2/event-config.h>

#include <fcntl.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/select.h>

#include <unistd.h>

#include "client.h"
#include "ip_parser.h"
#include "tun.h"
#include "utils.h"

#define SERVER_PORT 6666
#define SERVER_ADDRESS "127.0.0.1"
#define ACCESS_DENIED "Access denied"
#define TUN_NAME "tun0"
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

int send_id(client_t* client) {
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
    dest_addr.sin_port = htons(SERVER_PORT);
    if (inet_aton(SERVER_ADDRESS, &dest_addr.sin_addr) == 0) {
        goto error;
    }

    if (connect(sock, (struct sockaddr *) &dest_addr, sizeof(dest_addr)) == -1) {
        goto error;
    }
    puts("Client connected");

    return SUCCESS;

error:
    printf("Error occured while connecting to server: %s\n", strerror(errno));
	return FAILURE;
}

int routing(network_addr_t* web_addr, char* name) {
    char command[255];
    char str_mask[4];

    strcpy(command, "ip route add ");
    strcat(command, web_addr->network_addr);
    strcat(command, "/");
    sprintf(str_mask, "%d", web_addr->mask);
    strcat(command, str_mask);
    strcat(command, " dev ");
    strcat(command, name);
    if (system(command) != 0) {
        puts(command);
        return FAILURE;
    }

    return SUCCESS;
}

void read_tun_event_handler(int tun_socket, short flags, struct recv_param_s* recv_tun_param) {
    if (flags == 0) {
        perror("flags");
        exit(1);
    }

    char buffer[BUFSIZE];

    fcntl(tun_socket, F_SETFL, O_NONBLOCK);

    if (read(tun_socket, buffer, BUFSIZE) == -1) {
        perror("read()");
        exit(1);
    }

    if (strlen(buffer) == 0) {
        return;
    }

    puts("Got something in tunnel recv event handler!");
    puts(buffer);

    if (send_all(recv_tun_param->socket, buffer, strlen(buffer)) == -1) {
        perror("send_all()");
        exit(1);
    }
}

void recv_clt_event_handler(int client_server_socket, short flags, struct recv_param_s* recv_clt_param) {
    puts("Got something in client recv event handler!");

    if (flags == 0) {
        perror("flags");
        exit(1);
    }

    char buffer[BUFSIZE];

    if (recv_all(client_server_socket, BUFSIZE, buffer) == -1) {
        perror("recv_all()");
        exit(1);
    }
    if (write(recv_clt_param->socket, buffer, strlen(buffer)) == -1) {
        perror("write()");
        exit(1);
    }
}

int event_anticipation(int tun_socket, int client_server_socket) {
    
    struct event_base* base = event_base_new();
    if (!base) {
        goto error;
    }

    puts("Start event anticipation...");

    struct recv_param_s recv_tun_param = { client_server_socket };

    struct event* read_tun_event = event_new(base, tun_socket, EV_READ | EV_PERSIST, (event_callback_fn)read_tun_event_handler, (void*)&recv_tun_param);
    if (!read_tun_event) {
        goto error;
    }

    if (event_add(read_tun_event, NULL) < 0) {
        goto error;
    }

    struct recv_param_s recv_clt_param = { tun_socket };
    struct event* recv_clt_event = event_new(base, client_server_socket, EV_READ | EV_PERSIST, (event_callback_fn)recv_clt_event_handler, (void*)&recv_clt_param);
    if (!recv_clt_event) {
        goto error;
    }

    if (event_add(recv_clt_event, NULL) < 0) {
        goto error;
    }

    if (event_base_dispatch(base) < 0) {
        goto error;
    }

    event_base_free(base);

    return SUCCESS;

error:
    printf("Error occured while running event anticipation: %s\n", strerror(errno));
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
        return FAILURE;
    }
    puts("Client created");


    if (connect_to_server(client->sock) == FAILURE) {
        goto error;
    }

    if (send_id(client) == -1) {
        goto error;
    }
    char* get_addr = get_response(client);

    if (get_addr == NULL) {
        goto error;
    }

    if (strncmp(get_addr, ACCESS_DENIED, strlen(get_addr)) == 0) {
        printf("%s", ACCESS_DENIED);
        free(client);
	    return FAILURE;
    }

    int mask;
    char network_addr[255] = {};
    get_network_and_mask(get_addr, network_addr, &mask);

    // network_addr_t web_addr = { network_addr, mask };

    int tun_socket = create_client_tun(TUN_NAME, get_addr);


    // if (routing(&web_addr, TUN_NAME) == -1) {
    //     goto error;
    // }

    if (event_anticipation(tun_socket, client->sock) == FAILURE) {
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
