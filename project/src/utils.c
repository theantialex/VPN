#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

int send_all(int sock, char* str, int len) {
    int i = 0;
    for (; i < len; i++) {
        if (send(sock, &str[i], sizeof(str[i]), 0) == -1) {
            goto error;
        }
    }

    return SUCCESS;

error:
    printf("Error occured while writing from socket: %s\n", strerror(errno));
	return FAILURE;
}

int recv_all(int sock, int len, char* result_str) {
	char c;
	int i;
	for (i = 0; i < len; ++i) {
		if (recv(sock, &c, 1, 0) == -1) {
			goto error;
		}
		result_str[i] = c;
	}
	result_str[i] = '\0';
	return SUCCESS;

error:
    printf("Error occured while reading from socket: %s\n", strerror(errno));
	return FAILURE;
}

void free_client_id (client_id* id) {
    free(id->name);
    free(id->password);
}
