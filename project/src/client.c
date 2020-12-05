#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
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

#include "client.h"
#include "ip_parser.h"
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

// unsigned short csum(unsigned short *ptr,int nbytes) 
// {
// 	register long sum;
// 	unsigned short oddbyte;
// 	register short answer;

// 	sum=0;
// 	while(nbytes> 1) {
// 		sum+=*ptr++;
// 		nbytes-=2;
// 	}
// 	if(nbytes==1) {
// 		oddbyte=0;
// 		*((u_char*)&oddbyte)=*(u_char*)ptr;
// 		sum+=oddbyte;
// 	}

// 	sum = (sum>>16)+(sum & 0xffff);
// 	sum = sum + (sum>>16);
// 	answer=(short)~sum;
	
// 	return(answer);
// }

// struct pseudo_header
// {
// 	u_int32_t source_address;
// 	u_int32_t dest_address;
// 	u_int8_t placeholder;
// 	u_int8_t protocol;
// 	u_int16_t tcp_length;
// };

//  ip_packet_generation(char* source_ip, struct sockaddr_in sin, int source_port, int dest_port) {
//     char datagram[4096] , source_ip[32] , *data , *pseudogram;
	
// 	//zero out the packet buffer
// 	memset (datagram, 0, 4096);
	
// 	//IP header
// 	struct iphdr *iph = (struct iphdr *) datagram;
	
// 	//TCP header
// 	struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof (struct ip));
// 	struct pseudo_header psh;
	
// 	//Data part
// 	data = datagram + sizeof(struct iphdr) + sizeof(struct tcphdr);
// 	strcpy(data , "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	
// 	//Fill in the IP Header
// 	iph->ihl = 5;
// 	iph->version = 4;
// 	iph->tos = 0;
// 	iph->tot_len = sizeof (struct iphdr) + sizeof (struct tcphdr) + strlen(data);
// 	iph->id = htonl (54321);	//Id of this packet
// 	iph->frag_off = 0;
// 	iph->ttl = 255;
// 	iph->protocol = IPPROTO_TCP;
// 	iph->check = 0;		//Set to 0 before calculating checksum
// 	iph->saddr = inet_addr ( source_ip );	//Spoof the source ip address
// 	iph->daddr = sin.sin_addr.s_addr;
	
// 	//Ip checksum
// 	iph->check = csum ((unsigned short *) datagram, iph->tot_len);
	
// 	//TCP Header
// 	tcph->source = htons (source_port);
// 	tcph->dest = htons (dest_port);
// 	tcph->seq = 0;
// 	tcph->ack_seq = 0;
// 	tcph->doff = 5;	//tcp header size
// 	tcph->fin=0;
// 	tcph->syn=1;
// 	tcph->rst=0;
// 	tcph->psh=0;
// 	tcph->ack=0;
// 	tcph->urg=0;
// 	tcph->window = htons (5840);	/* maximum allowed window size */
// 	tcph->check = 0;	//leave checksum 0 now, filled later by pseudo header
// 	tcph->urg_ptr = 0;
	
// 	//Now the TCP checksum
// 	psh.source_address = inet_addr( source_ip );
// 	psh.dest_address = sin.sin_addr.s_addr;
// 	psh.placeholder = 0;
// 	psh.protocol = IPPROTO_TCP;
// 	psh.tcp_length = htons(sizeof(struct tcphdr) + strlen(data) );
	
// 	int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + strlen(data);
// 	pseudogram = malloc(psize);
	
// 	memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
// 	memcpy(pseudogram + sizeof(struct pseudo_header) , tcph , sizeof(struct tcphdr) + strlen(data));
	
// 	tcph->check = csum( (unsigned short*) pseudogram , psize);
	
//     return iph;	
// 	//loop if you want to flood :)
// }


int routing(int post_sock, char* network_addr, int net_mask) {
    struct sockaddr_in dest_addr;
    socklen_t dest_addr_len;
    if (getsockname(post_sock, (struct sockaddr*)&dest_addr, &dest_addr_len) == -1) {
        goto error;
    }

    return is_in_network(network_addr, net_mask, inet_ntoa(dest_addr.sin_addr));

error:
    printf("Error occured while routing: %s\n", strerror(errno));
    return FAILURE;
}

// int accept_event_handler(int sock) {
//     int post_socket;
//     socklen_t remotelen;
//     struct sockaddr_in remote;
//     char buffer[BUFSIZE];

//     remotelen = sizeof(remote);
//     memset(&remote, 0, remotelen);
//     if (post_socket = accept(sock, (struct sockaddr*)& remote, &remotelen) < 0){
//       perror("accept()");
//       exit(1);
//     }
    
//     if (recv_all(post_socket, BUFSIZE, buffer) == -1) {
//             goto error;
//     }

//     //ROUTING
//     getsockname(p)
    
//     // if (routing(...) == 1) {

//     //     if (send_all(sock, strlen(buffer), buffer) == -1) {
//     //         goto error;
//     //     }
//     // }

// error:
//     printf("Error occured while running accept event handler: %s\n", strerror(errno));
// 	return FAILURE;
// }

// int recv_event_handler(int tun_socket, int client_server_socket) {
//     char buffer[BUFSIZE];

//     if (recv_all(client_server_socket, BUFSIZE, buffer) == -1) {
//         goto error;
//     }

//     if (send_all(tun_socket, strlen(buffer), buffer) == -1) {
//         goto error;
//     }

//     error:
//     printf("Error occured while running accept event handler: %s\n", strerror(errno));
// 	return FAILURE;
// }

// int event_anticipation(int tun_socket, int client_server_socket) {


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

//    }
// error:
//     printf("Error occured while running event anticipation: %s\n", strerror(errno));
// 	return FAILURE;
// }

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

    struct sockaddr_in dest_addr;
    socklen_t dest_addr_len = sizeof(dest_addr);

    if (getsockname(clt_socket, (struct sockaddr*)&dest_addr, &dest_addr_len) == -1) {
        puts("here");
        goto error;
    }


    // int tun_socket = create_client_tun(TUN_NAME, SERVER_PORT, SERVER_ADDRESS);
    // if (event_anticipation(tun_socket, client->sock) != 0) {
    //     goto error;
    // }

    free(get_addr);
    free(client);

    return SUCCESS;

error:
    printf("Error occured while running client: %s\n", strerror(errno));
    free(client);
	return FAILURE;
}
