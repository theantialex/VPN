#include <arpa/inet.h>
#include <errno.h>
#include <event2/event.h>
#include <event2/event-config.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "ip_parser.h"
#include "server.h"
#include "tun.h"

#define active_clients_fpath "./project/data/active_clients.txt"
#define config_fpath "./project/data/config.txt"
#define MAX_STORAGE 250
#define BUFSIZE 16536

typedef enum
{
	WRONG_CMD_ID = -1,
	CREATE_CMD_ID = 0,
	CONNECT_CMD_ID = 1,
	LEAVE_CMD_ID = 2,
	DELETE_CMD_ID = 3,
} CMD;

// volatile int process_exited = 0;

static client_in_t *client_db[MAX_STORAGE][MAX_STORAGE] = {};

int available_client_in_nw[MAX_STORAGE][MAX_STORAGE] = {};
int available_network[MAX_STORAGE] = {};

// static void sigterm_handler(int signum) {
// 	if (signum) {
// 		process_exited = 1;
// 	} else {
// 		process_exited = 1;
// 	}
// }

server_t *server_create(hserver_config_t *config)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		printf("Error occured: %s\n", strerror(errno));
		return NULL;
	}

	int enable = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
	{
		goto error;
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(config->port);
	addr.sin_addr.s_addr = INADDR_ANY;

	puts("Server created");

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		goto error;
	}

	if (listen(sock, config->backlog) == -1)
		goto error;

	puts("Listening...");

	server_t *server = calloc(1, sizeof(server_t));
	if (server == NULL)
	{
		goto error;
	}

	server->sock = sock;

	FILE *config_file = fopen(config_fpath, "rb");
	if (getc(config_file) == EOF)
	{
		FILE *config_file = fopen(config_fpath, "w");
		if (config_file == NULL)
		{
			return NULL;
		}
		fclose(config_file);
	}

	FILE *active_clients_file = fopen(active_clients_fpath, "w");
	if (active_clients_file == NULL)
	{
		free(server);
		goto error;
	}
	fclose(active_clients_file);

	server->base = event_base_new();
	return server;

error:
	printf("Error occured while creating server: %s\n", strerror(errno));
	close(sock);
	return NULL;
}

// int process_setup_signals() {
// 	struct sigaction action = {0};
// 	action.sa_handler = sigterm_handler;
// 	if (sigaction(SIGTERM, &action, NULL) == -1) {
// 		goto error;
// 	}

// 	return SUCCESS;

// error:
// 	printf("Error occured while setting up signals: %s\n", strerror(errno));
// 	return FAILURE;
// }

// int add_active_client(char* client_name, char* client_network) {
// 	FILE* aclient_file = fopen(active_clients_fpath, "a+");
// 	if (aclient_file == NULL) {
// 		goto error;
// 	}

// 	if (fprintf(aclient_file, "%s %s\n", client_name, client_network) < 0) {
// 		fclose(aclient_file);
// 		goto error;
// 	}

// 	fclose(aclient_file);
// 	return SUCCESS;

// error:
// 	printf("Error occured while adding active client: %s\n", strerror(errno));
// 	return FAILURE;
// }

// client_id* get_client_id(int socket) {
// 	int id_len;
// 	if (recv(socket, &id_len, sizeof(id_len), 0) == -1) {
// 		goto error;
// 	}

// 	client_id* id = calloc(1, sizeof(client_id));
// 	if (id == NULL) {
// 		goto error;
// 	}

// 	id->name = (char*) calloc(id_len, sizeof(char));
// 	if (id->name == NULL) {
// 		free(id);
// 		goto error;
// 	}

// 	if (recv_all(socket, id_len, id->name) == -1) {
// 		free(id->name);
// 		free(id);
// 		goto error;
// 	}

// 	if (recv(socket, &id_len, sizeof(id_len), 0) == -1) {
// 		free(id->name);
// 		free(id);
// 		goto error;
// 	}

// 	id->password = (char*) calloc(id_len,sizeof(char));
// 	if (id->password == NULL) {
// 		free(id->name);
// 		free(id);
// 		goto error;
// 	}

// 	if (recv_all(socket, id_len, id->password) == -1) {
// 		free_client_id(id);
// 		free(id);
// 		goto error;
// 	}

// 	return id;

// error:
// 	printf("Error occured while getting client id: %s\n", strerror(errno));
// 	return NULL;
// }

// int get_client_access(client_id* id, int network_storage_id, char* access_result) {
// 	if (client_identify(id, access_result) == -1) {
// 		strcpy(access_result, "Access denied");

// 		return -1;
// 	}

// 	int client_storage_id = get_first_available_client(network_storage_id);

// 	if (client_storage_id == -1) {
// 		strcpy(access_result, "Access denied");
// 		puts("The network is full. No access given");
// 		return -1;
// 	}

// 	add_active_client(id->name, access_result);

// 	return client_storage_id;
// }

// int send_access_to_client(client_id* id, int socket, int network_storage_id, int* client_storage_id) {
// 	char access_result[MAX_STORAGE];

// 	*client_storage_id = get_client_access(id, network_storage_id, access_result);

// 	size_t access_res_len = strlen(access_result);

// 	if (send(socket, &access_res_len, sizeof(access_res_len), 0) == -1) {
// 		goto error;
// 	}

// 	if (send_all(socket, access_result, access_res_len) == -1) {
// 		goto error;
// 	}

// 	return SUCCESS;

// error:
// 	printf("Error occured while sending access to client: %s\n", strerror(errno));
// 	return FAILURE;
// }

int get_first_available_client(int network_id)
{
	for (int i = 0; i < MAX_STORAGE; i++)
	{
		if (available_client_in_nw[network_id][i] == 0)
		{
			return i;
		}
	}
	return -1;
}

int get_first_available_network()
{
	for (int i = 0; i < MAX_STORAGE; i++)
	{
		if (available_network[i] == 0)
		{
			return i;
		}
	}
	return -1;
}

// int client_identification_process(int client_sock, int server_sock, hserver_config_t* server_param, storage_id_t* clt_db_id) {
// 	client_id* id = get_client_id(client_sock);
// 	if (id == NULL) {
// 		goto error;
// 	}

// 	printf("Got client id: %s %s\n", id->name, id->password);

// 	int client_storage_id;
// 	if (send_access_to_client(id, client_sock, clt_db_id->network_id, &client_storage_id) == -1) {
// 		free_client_id(id);
// 		free(id);

// 		goto error;
// 	};

// 	if (client_storage_id != -1) {
// 		char* tun_name = "server_tun";
// 		char tun_id_str[10] = {};
// 		int post_sock;
// 		sprintf(tun_id_str, "%d", client_storage_id);

// 		char res_name[20];
// 		snprintf(res_name, 20, "%s%s", tun_name, tun_id_str);

// 		char access_res[MAX_STORAGE];
// 		get_client_access(id, clt_db_id->network_id, access_res);

// 		int tun_origin_sock = create_server_tun(res_name, server_param, access_res);
// 		if (tun_origin_sock == -1) {
// 			goto error;
// 		}

// 		post_sock = accept(server_sock, NULL, NULL);

// 		client_in_t* client_in = calloc(1, sizeof(client_in_t));
// 		client_in->client_socket = client_sock;

// 		client_in->tun_socket = post_sock;

// 		clt_db_id->client_id = client_storage_id;

// 		client_db[clt_db_id->network_id][clt_db_id->client_id] = client_in;
// 		available_client_in_nw[clt_db_id->network_id][client_storage_id] = 1;

// 	}

// 	free_client_id(id);
// 	free(id);

// 	return SUCCESS;

// error:
// 	printf("Error occured while running client identification process: %s\n", strerror(errno));
// 	return FAILURE;
// }

int add_network(network_id_t net_id, int *network_db_id)
{
	FILE *config_file = fopen(config_fpath, "a+");
	if (config_file == NULL)
	{
		goto error;
	}

	int net_storage_id = get_first_available_network();
	*network_db_id = net_storage_id;
	if (net_storage_id == -1)
	{
		puts("Server is full. Can't allocate network");
		return FAILURE;
	}

	char net_sid_str[10] = {};
	sprintf(net_sid_str, "%d", net_storage_id);

	char network_addr[MAX_STORAGE];
	strcpy(network_addr, "10.");
	strcat(network_addr, net_sid_str);
	strcat(network_addr, ".0.0/24");

	if (fprintf(config_file, "%s %s %s\n", net_id.name, net_id.password, network_addr) < 0)
	{
		fclose(config_file);
		goto error;
	}

	available_network[net_storage_id] = 1;

	printf("Allocated network %s\n", network_addr);

	fclose(config_file);
	return SUCCESS;

error:
	printf("Error occured while adding active client: %s\n", strerror(errno));
	return FAILURE;
}

int client_identify(network_id_t *id, char *network_addr)
{
	FILE *config_file = fopen(config_fpath, "r");
	if (config_file == NULL)
	{
		printf("Error occured while identifying client: %s\n", strerror(errno));
		return FAILURE;
	}

	char net_name[MAX_STORAGE];
	char net_password[MAX_STORAGE];
	char net_addr[MAX_STORAGE];

	while (fscanf(config_file, "%s %s %s", net_name, net_password, net_addr) == 3)
	{
		if (strlen(net_name) == strlen(id->name) && strncmp(net_name, id->name, strlen(net_name)) == 0)
		{
			if (strlen(net_password) == strlen(id->password) && strncmp(net_password, id->password, strlen(net_password)) == 0)
			{
				strncpy(network_addr, net_addr, strlen(net_addr));

				fclose(config_file);
				return SUCCESS;
			}
		}
	}
	fclose(config_file);
	return FAILURE;
}

int connect_process(network_id_t *network_id, int server_socket, int client_socket, storage_id_t *client_db_id, char *response)
{
	char access_result[MAX_STORAGE];

	if (client_identify(network_id, access_result) == FAILURE)
	{
		strcpy(access_result, "Access denied");
		client_db_id->network_id = -1;
		client_db_id->client_id = -1;
		return FAILURE;
	}

	puts("here");

	if (server_socket == -1)
	{
		return FAILURE;
	}

	int octs[4];
	int mask;
	get_octs_and_mask_from_ip(access_result, octs, &mask);
	int net_storage_id = octs[1];

	puts("here");

	int client_storage_id = get_first_available_client(net_storage_id);
	if (client_storage_id == -1)
	{
		strcpy(access_result, "Access denied");
		puts("The network is full. No access given");
		client_db_id->network_id = -1;
		client_db_id->client_id = -1;
		return FAILURE;
	}

	octs[3] = client_storage_id + 1;
	sprintf(access_result, "%d.%d.%d.%d/%d", octs[0], octs[1], octs[2], octs[3], mask);

	if (client_storage_id != -1)
	{
		char *tun_name = "server_tap";
		char tun_id_str[10] = {};
		// 	int post_sock;
		sprintf(tun_id_str, "%d", client_storage_id);

		char res_name[20];

		char network_id_str[10] = {};
		sprintf(network_id_str, "%d", client_db_id->network_id);
		snprintf(res_name, 20, "%s%s", tun_name, network_id_str);
		strcat(res_name, "_");
		strcat(res_name, tun_id_str);

		int tun_origin_sock = create_server_tun(res_name, access_result);
		if (tun_origin_sock == -1)
		{
			goto error;
		}

		// post_sock = accept(server_socket, NULL, NULL);

		client_in_t *client_in = calloc(1, sizeof(client_in_t));
		client_in->client_socket = client_socket;

		//client_in->tun_socket = post_sock;
		client_in->tun_socket = tun_origin_sock;

		client_db_id->network_id = net_storage_id;
		client_db_id->client_id = client_storage_id;

		client_db[net_storage_id][client_storage_id] = client_in;
		available_client_in_nw[net_storage_id][client_storage_id] = 1;
	}

	strncpy(response, access_result, MAX_STORAGE);
	return SUCCESS;

error:
	client_db_id->network_id = -1;
	client_db_id->client_id = -1;
	printf("Error occured while running client connection process: %s\n", strerror(errno));
	return FAILURE;
}

int process_cmd(int clt_sock, int server_sock, int *cmd_id, char *response, storage_id_t *clt_db_id)
{
	puts("Started processing cmd...");
	int cmd_len;
	if (recv(clt_sock, &cmd_len, sizeof(cmd_len), 0) == -1)
	{
		goto error;
	}
	char *cmd = (char *)calloc(cmd_len, sizeof(char));
	if (cmd == NULL)
	{
		goto error;
	}
	if (recv_all(clt_sock, cmd_len, cmd) == -1)
	{
		free(cmd);
		goto error;
	}

	network_id_t net_id;
	int net_name_len;
	if (recv(clt_sock, &net_name_len, sizeof(net_name_len), 0) == -1)
	{
		goto error;
	}

	net_id.name = calloc(MAX_STORAGE, sizeof(char));
	if (recv_all(clt_sock, net_name_len, net_id.name) == -1)
	{
		goto error;
	}

	int net_pswd_len;
	if (recv(clt_sock, &net_pswd_len, sizeof(net_pswd_len), 0) == -1)
	{
		goto error;
	}

	net_id.password = calloc(MAX_STORAGE, sizeof(char));
	if (recv_all(clt_sock, net_pswd_len, net_id.password) == -1)
	{
		goto error;
	}

	if (strncmp(cmd, CREATE_CMD, strlen(cmd)) == 0)
	{
		*cmd_id = CREATE_CMD_ID;
		if (add_network(net_id, &clt_db_id->network_id) == FAILURE)
		{
			strncpy(response, CREATE_FAILURE, 30);
		}
		else
		{
			strncpy(response, CREATE_SUCCESS, 30);
		}
	}
	else if (strncmp(cmd, CONNECT_CMD, strlen(cmd)) == 0)
	{
		*cmd_id = CONNECT_CMD_ID;
		if (connect_process(&net_id, server_sock, clt_sock, clt_db_id, response) == FAILURE)
		{
			puts("Can't process connect cmd. Something went wrong");
		}
	}

	// printf("Response for client: %s\n", response);

	return SUCCESS;

error:
	printf("Error occured while processing cmd: %s\n", strerror(errno));
	return FAILURE;
}

int send_cmd_response(int sock, char *response)
{
	int len = strlen(response);
	if (send(sock, &len, sizeof(len), 0) == -1)
	{
		goto error;
	}

	if (send_all(sock, response, len) == -1)
	{
		goto error;
	}

	printf("Sent response: %s\n", response);
	return SUCCESS;

error:
	printf("Error occured while sending cmd response: %s\n", strerror(errno));
	return FAILURE;
}

int accept_event_handler(int server_sock, int *client_server_socket, int *cmd_id, char *response, storage_id_t *clt_db_id)
{
	fcntl(server_sock, F_SETFL, O_NONBLOCK);

	struct sockaddr_in client;
	socklen_t len = sizeof(client);
	if ((*client_server_socket = accept(server_sock, (struct sockaddr *)&client, &len)) <= 0)
	{
		return FAILURE;
	}

	char s[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET, &(((struct sockaddr_in *)&client)->sin_addr), s, sizeof(s));
	printf("Got a connection from %s\n", s);

	if (*client_server_socket == -1)
	{
		goto error;
	}

	if (process_cmd(*client_server_socket, server_sock, cmd_id, response, clt_db_id) == FAILURE)
	{
		goto error;
	}

	return SUCCESS;

error:
	printf("Error occured while accepting event handler: %s\n", strerror(errno));
	return FAILURE;
}

void client_recv_event_handler(int client_server_socket, short flags, struct clt_recv_param_s *params)
{
	puts("Got something in clt_recv event handler!");

	if (flags == 0)
	{
		perror("flags");
		exit(1);
	}

	char buffer[BUFSIZE];

	int n = recv(client_server_socket, buffer, BUFSIZE, 0);
	if (n == -1)
	{
		perror("read()");
		exit(1);
	}
	printf("Received %d of data\n", n);

	int m = write(params->server_tun_socket, buffer, n);
	if (m == -1)
	{
		perror("write()");
		exit(1);
	}
	printf("Sent %d of data\n", m);
}

void tun_recv_event_handler(int tun_socket, short flags, struct tun_recv_param_s *params)
{
	puts("Got something in tun_recv event handler!");

	if (flags == 0)
	{
		perror("flags");
		exit(1);
	}

	char buffer[BUFSIZE];

	int n = read(tun_socket, buffer, BUFSIZE);
	if (n == -1)
	{
		perror("read()");
		exit(1);
	}
	printf("Received %d of data\n", n);
	// int i = 0;
	// puts("Packet:");
	// while(i < n) {
	// 	printf("%02x", buffer[i]);
	// 	i++;
	// }
	// puts("");

	int m = send(params->client_socket, buffer, n, 0);
	if (m == -1)
	{
		perror("write()");
		exit(1);
	}
	printf("Sent %d of data\n", m);
}

int create_bridge(storage_id_t *db_id)
{
	char command[255];
	char bridge_id_str[10] = {};
	sprintf(bridge_id_str, "%d", db_id->network_id);

	strcpy(command, "brctl addbr bridge");
	strcat(command, bridge_id_str);

	if (system(command) != 0)
	{
		return FAILURE;
	}

	printf("Bridge %s created\n", bridge_id_str);
	return SUCCESS;
}

int add_tun_to_bridge(storage_id_t *db_id)
{
	char command[255];

	char bridge_id_str[10] = {};
	sprintf(bridge_id_str, "%d", db_id->network_id);

	char tap_id_str[10] = {};
	sprintf(tap_id_str, "%d", db_id->client_id);

	strcpy(command, "brctl addif bridge");
	strcat(command, bridge_id_str);
	strcat(command, " server_tap");
	strcat(command, bridge_id_str);
	strcat(command, "_");
	strcat(command, tap_id_str);

	if (system(command) != 0)
	{
		return FAILURE;
	}

	printf("Bridge device server_tap%s_%s added\n", bridge_id_str, tap_id_str);
	return SUCCESS;
}

static int n_clients = 0;

int event_anticipation(server_t *server, storage_id_t *clt_db_id)
{
//	struct event_base *ev_base_arr[250];
	int i = 0;
//	int j = 0;

	int cmd_id;
	char *response = (char *)calloc(MAX_STORAGE, sizeof(char));
	while (n_clients < 2)
	{
		int client_server_socket;
		if (accept_event_handler(server->sock, &client_server_socket, &cmd_id, response, clt_db_id) == SUCCESS)
		{
			if (cmd_id == CREATE_CMD_ID)
			{
				// if (create_bridge(clt_db_id) == FAILURE)
				// {
				// 	goto error;
				// }
				if (send_cmd_response(client_server_socket, response) == FAILURE)
				{
					goto error;
				}
			}
			if (cmd_id == CONNECT_CMD_ID)
			{
				// if (add_tun_to_bridge(clt_db_id) == FAILURE)
				// {
				// 	goto error;
				// }
				if (clt_db_id->network_id == -1 || clt_db_id->client_id == -1)
				{
					exit(1);
				}
				int client_server_sock = client_db[clt_db_id->network_id][clt_db_id->client_id]->client_socket;
				int tunnel_sock = client_db[clt_db_id->network_id][clt_db_id->client_id]->tun_socket;
				printf("client sock %d tunnel sock %d\n", client_server_sock, tunnel_sock);

				struct event_base *base = server->base; 
				if (!base)
				{
					perror("base");
					exit(1);
				}

				struct clt_recv_param_s *clt_param = malloc(sizeof(struct clt_recv_param_s));
				clt_param->server_tun_socket = tunnel_sock;

				struct event *clt_recv_event = event_new(base, client_server_sock, EV_READ | EV_PERSIST,
														 (event_callback_fn)client_recv_event_handler, (void *)clt_param);
				if (!clt_recv_event)
				{
					goto error;
				}
				if (event_add(clt_recv_event, NULL) < 0)
				{
					goto error;
				}

				struct tun_recv_param_s *tun_param = malloc(sizeof(struct tun_recv_param_s));
				tun_param->client_socket = client_server_sock;
				struct event *tun_recv_event = event_new(base, tunnel_sock, EV_READ | EV_PERSIST,
														 (event_callback_fn)tun_recv_event_handler, (void *)tun_param);
				if (!tun_recv_event)
				{
					goto error;
				}
				if (event_add(tun_recv_event, NULL) < 0)
				{
					goto error;
				}

//				ev_base_arr[i] = base;
				i++;

				if (send_cmd_response(client_server_socket, response) == FAILURE)
				{
					goto error;
				}
				n_clients++;
			}
		}
    }

		printf("dispatch\n");
    if (event_base_dispatch(server->base) < 0) {
        goto error;
/*		j = 0;
		while (j < i)
		{
			if (ev_base_arr[j] != NULL)
			{
				event_base_loop(ev_base_arr[j], EVLOOP_NONBLOCK | EVLOOP_ONCE);
				// printf("loop %d\n", j);
			}
			j++;
		}
		*/
	}

	return SUCCESS;

error:
	printf("Error occured while running event anticipation: %s\n", strerror(errno));
	return FAILURE;
}

int server_run(hserver_config_t *config)
{
	server_t *server = server_create(config);
	if (server == NULL)
	{
		goto error;
	}

	storage_id_t client_db_id;

	if (event_anticipation(server, &client_db_id) == FAILURE)
	{
		goto error;
	}

	server_close(server);
	return SUCCESS;

error:
	return FAILURE;
}

void server_close(server_t *server)
{
	close(server->sock);
	free(server);
	free_client_db();
}

void free_client_db()
{
	for (int i = 0; i < MAX_STORAGE; i++)
	{
		for (int j = 0; j < MAX_STORAGE; j++)
		{
			if (client_db[i][j] != NULL)
			{
				free(client_db[i][j]);
			}
		}
	}
}
