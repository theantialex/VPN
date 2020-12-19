#include <string.h>

#include "client.h"

int main(int argc, char *argv[]) {

    if (argc < 4) {
        return -1;
    }

    network_id_t net_id = {argv[2], argv[3]};
    char* param[100];
    if (argc > 4) {
        for (int i = 0, j = 4; j < argc; j++, i++) {
            param[i] = calloc(100, sizeof(char));
            strncpy(param[i], argv[j], strlen(argv[j]));
        }
    }

    return client_run_cmd(argv[1], net_id, param);
}