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
#define client_data_dir "./project/client_data"
#define net_config_fpath "./project/client_data/net_config.txt"

#define ACCESS_DENIED "Access denied"
#define TUN_NAME "tun0"
#define BUFSIZE 16536

client_t* client_create(int socket, network_id_t net_id) {
    client_t* client = calloc(1, sizeof(*client));
    if (client == NULL) {

        return NULL;
    }
    client->net_id = net_id;
    client->sock = socket;

    if (opendir(client_data_dir) == NULL) {
        if (mkdir(client_data_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
            return NULL;
        }
    }
    if (fopen(net_config_fpath, "r") == NULL) {
        FILE* net_config_file = fopen(net_config_fpath, "w");
        if (net_config_file == NULL) {
            return NULL;
        }
        fclose(net_config_file);
    }
    
    return client;
}

int connect_to_server(int sock, char* server_addr) {
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);
    printf("%s %ld %d\n", server_addr, strlen(server_addr), server_addr[strlen(server_addr)-1]);
    if (inet_aton(server_addr, &dest_addr.sin_addr) == 0) {
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
    puts("Got something in tunnel recv event handler!");
    memcpy(ip_packet, buffer, n);
    packet_len = n;

    // while (n != -1) {
    //     n = read(tun_socket, buffer, BUFSIZE);
    //     strncat(ip_packet, buffer, n);
    //     packet_len += n;
    // }

    printf("ip p = %d\n", packet_len);
    if (write(recv_tun_param->socket, ip_packet, packet_len) == -1) {
        perror("send()");
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
    int n = read(client_server_socket, buffer, BUFSIZE);
    if (n == -1) {
        perror("recv_all()");
        exit(1);
    }
    printf("ip p = %d\n", n);

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

int client_save_data(network_id_t net_id, char* server_addr) {
    FILE* net_config_file = fopen(net_config_fpath, "a+");
    if (net_config_file == NULL) {
		goto error;
	}

	if (fprintf(net_config_file, "%s %s\n",net_id.name, server_addr) < 0) {
		fclose(net_config_file);
		goto error;
	}

	fclose(net_config_file);

    return SUCCESS;
error:
	printf("Error occured while saving client data: %s\n", strerror(errno));
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

int get_server_addr(network_id_t* net_id, char* server_addr) {
    FILE* net_config_file = fopen(net_config_fpath, "r");
    char net_name[MAX_STORAGE];
    char conf_server_addr[MAX_STORAGE];

    while(fscanf(net_config_file, "%s %s", net_name, conf_server_addr) == 2) {
		if (strncmp(net_name, net_id->name, strlen(net_name)) == 0) {
            strncpy(server_addr, conf_server_addr, strlen(conf_server_addr));

            fclose(net_config_file);
            return SUCCESS;
		}
	}
    fclose(net_config_file);
    return FAILURE;
}

int network_creation_response(char* response) {
    if (strncmp(response, CREATE_SUCCESS, strlen(response)) != 0) {
            return FAILURE;
    }
    return SUCCESS;
}

int set_connection_process(char* get_addr, int socket) {
    if (strncmp(get_addr, ACCESS_DENIED, strlen(get_addr)) == 0) {
        return FAILURE;
    }

    int mask;
    char network_addr[255] = {};
    get_network_and_mask(get_addr, network_addr, &mask);
    
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
        puts("well");
        goto error;
    }

    if (send_cmd_request(cmd, client) == FAILURE) {
        goto error;
    }
    char* response = (char*) calloc(MAX_STORAGE, sizeof(char));
    if (get_cmd_response(client->sock, response) == FAILURE) {
        goto error;
    }

    if (cmd_choice(cmd, response, client->sock) == FAILURE) {
        goto error;
    }  

    return SUCCESS;
error:
    printf("Error occured while processing command: %s\n", strerror(errno));
    free(client);
    free(server_addr);
	return FAILURE;

}
