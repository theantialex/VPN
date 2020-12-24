#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <event2/event.h>
#include <event2/event-config.h>

#include <fcntl.h>
#include <ifaddrs.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
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
#define pid_file_fpath "./project/data/pid.txt"
#define ACCESS_DENIED "Access denied"
#define TUN_NAME "tap0"
#define BUFSIZE 16536

// static client_event_t* client_event;

client_t* client_create(int socket, network_id_t net_id) {
    client_t* client = calloc(1, sizeof(*client));
    if (client == NULL) {

        return NULL;
    }
    client->net_id = net_id;
    client->sock = socket;
    
    return client;
}

int connect_kill() {
    char command[255];
	char pid_str[10] = {};

    FILE* pid_file = fopen(pid_file_fpath, "r");
	fscanf(pid_file, "%s", pid_str);
    fclose(pid_file);

	strcpy(command, "kill ");
	strcat(command, pid_str);
    if (system(command) != 0) {
		return FAILURE;
	}

    return SUCCESS;
}

int connect_to_server(int sock, char* server_addr) {
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);
    if (inet_aton(server_addr, &dest_addr.sin_addr) == 0) {
        goto error;
    }

    if (connect(sock, (struct sockaddr *) &dest_addr, sizeof(dest_addr)) == -1) {
        goto error;
    }
    puts("Client connected to server");

    return SUCCESS;

error:
    printf("Error occured while connecting to server: %s\n", strerror(errno));
	return FAILURE;
}

void read_tun_event_handler(int tun_socket, short flags, struct recv_param_s* recv_tun_param) {
    if (flags == 0) {
        perror("flags");
        exit(1);
    }

    char buffer[BUFSIZE];
    char* ip_packet = calloc(BUFSIZE, sizeof(char));
    int packet_len = 0;

    fcntl(tun_socket, F_SETFL, O_NONBLOCK);

    int n = read(tun_socket, buffer, BUFSIZE);
    if (n == -1) {
        perror("read()");
        exit(1);
    }
    puts("Received ip packet on tap interface");
    memcpy(ip_packet, buffer, n);
    packet_len = n;

    // while (n != -1) {
    //     n = read(tun_socket, buffer, BUFSIZE);
    //     strncat(ip_packet, buffer, n);
    //     packet_len += n;
    // }

    printf("Read %d of data\n", packet_len);
    if (write(recv_tun_param->socket, ip_packet, packet_len) == -1) {
        perror("send()");
        exit(1);
    }
    printf("Sent %d of data\n", packet_len);
}

void recv_clt_event_handler(int client_server_socket, short flags, struct recv_param_s* recv_clt_param) {
    puts("Received ip packet on socket");

    if (flags == 0) {
        perror("flags");
        exit(1);
    }

    char buffer[BUFSIZE];
    int n = read(client_server_socket, buffer, BUFSIZE);
    if (n == -1) {
        perror("recv_all()");
        exit(1);
    }
    printf("Read %d of data\n", n);

    // printf("Received %d of data\n", n);
	// int i = 0;
	// puts("Packet:");
	// while(i < n) {
	// 	printf("%02x", buffer[i]);
	//     i++;
	// }
	// puts("");

    if (write(recv_clt_param->socket, buffer, n) == -1) {
        perror("write()");
        exit(1);
    }
    printf("Sent %d of data\n", n);
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

int send_cmd_request(char* cmd, client_t* client) {
    int len = strlen(cmd);
    if (send(client->sock, &len, sizeof(len), 0) == -1) {
        goto error;
    }

    if (send_all(client->sock, cmd, strlen(cmd)) == -1) {
        goto error;
    }

    len = strlen(client->net_id.name);
    if (send(client->sock, &len, sizeof(len), 0) == -1) {
        goto error;
    }

    if (send_all(client->sock, client->net_id.name, strlen(client->net_id.name)) == -1) {
        goto error;
    }

    len = strlen(client->net_id.password);
    if (send(client->sock, &len, sizeof(len), 0) == -1) {
        goto error;
    }

    if (send_all(client->sock, client->net_id.password, strlen(client->net_id.password)) == -1) {
        goto error;
    }

    puts("Sent cmd request");

    return SUCCESS;

error:
    printf("Error occured while sending cmd request: %s\n", strerror(errno));
	return FAILURE;
}

int network_creation_response(char* response) {
    if (strncmp(response, CREATE_SUCCESS, strlen(response)) != 0) {
            return FAILURE;
    }
    return SUCCESS;
}

int network_delete_response(char* response) {
    if (strncmp(response, DELETE_SUCCESS, strlen(response)) != 0) {
            return FAILURE;
    }
    return SUCCESS;
}

int network_disconnect_response(char* response) {
    if (strncmp(response, DISCONNECT_SUCCESS, strlen(response)) != 0) {
            return FAILURE;
    }
    return connect_kill();
}

int set_connection_process(char* get_addr, int socket) {
    if (strncmp(get_addr, ACCESS_DENIED, strlen(get_addr)) == 0 || strncmp(get_addr, EMPTY_RESPONSE, strlen(get_addr)) == 0) {
        return FAILURE;
    }

    FILE* pid_file = fopen(pid_file_fpath, "w");
    if (fprintf(pid_file, "%d", getpid()) < 0) {
        perror("write to pid.txt");
        return FAILURE;
    }
    fclose(pid_file);


    
    int tun_socket = create_client_tun(TUN_NAME, get_addr);
    if (event_anticipation(tun_socket, socket) == FAILURE) {
        return FAILURE;
    }
    return SUCCESS;
}

int get_cmd_response(int socket, char* response) {
    size_t response_len;
    if (recv(socket, &response_len, sizeof(response_len), 0) == -1) {
        goto error;
    }

    if (recv_all(socket, response_len, response) == -1) {
        free(response);
        goto error;
    }

    printf("Responce received: %s\n", response);
    return SUCCESS;

error:
    printf("Error occured while getting cmd response: %s\n", strerror(errno));
	return FAILURE;
}

int cmd_choice(char* cmd, char* response, int socket) {
    if (strncmp(cmd, CREATE_CMD, strlen(cmd)) == 0) {
        return network_creation_response(response);
    }
    if (strncmp(cmd, DELETE_CMD, strlen(cmd)) == 0) {
        return network_delete_response(response);
    }
    if (strncmp(cmd, DISCONNECT_CMD, strlen(cmd)) == 0) {
        return network_disconnect_response(response);
    }

    if (strncmp(cmd, CONNECT_CMD, strlen(cmd)) == 0) {
        if (set_connection_process(response, socket) == FAILURE) {
            puts("Error occured while setting connection");
            return FAILURE;
        }
        return SUCCESS;
    }
    return FAILURE;
}

int client_run_cmd(char* cmd, network_id_t net_id, char* param[]) {
    int clt_socket;
    clt_socket  = socket(AF_INET, SOCK_STREAM, 0);
    if (clt_socket == -1) {
        printf("Error occured while creating socket in client_cmd_run: %s\n", strerror(errno));
        return FAILURE;
    }

    client_t* client = client_create(clt_socket, net_id);

    if (client == NULL) {
        printf("Error occured while creating client in client_cmd_run: %s\n", strerror(errno));
        return FAILURE;
    }

    char* server_addr = (char*)calloc(MAX_STORAGE, sizeof(char));
    if (server_addr == NULL) {
        printf("Error occured while creating client in client_cmd_run: %s\n", strerror(errno));
        free(client);
        return FAILURE;
    }
    strncpy(server_addr, param[0], strlen(param[0]));

    if (connect_to_server(client->sock, server_addr) == FAILURE) {
        goto error;
    }

    if (send_cmd_request(cmd, client) == FAILURE) {
        goto error;
    }
    char* response = (char*) calloc(MAX_STORAGE, sizeof(char));
    if (get_cmd_response(client->sock, response) == FAILURE) {
        goto error;
    }

    if ((cmd_choice(cmd, response, client->sock) == FAILURE) && strncmp(strerror(errno), "Success", 8) != 0) {
        goto error;
    }  

    return SUCCESS;
error:
    printf("Error occured while processing command: %s\n", strerror(errno));
    free(client);
    free(server_addr);
	return FAILURE;

}
