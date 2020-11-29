#include "client.h"

int main(int argc, char *argv[]) {

    if (argc < 2) {
        return -1;
    }

    client_id id = {argv[1], argv[2]};

    return client_run(&id);
}