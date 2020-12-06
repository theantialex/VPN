#include "client.h"

int main(int argc, char *argv[]) {

    if (argc < 2) {
        return -1;
    }

    client_id id = {argv[1], argv[2]};

    int run_result = client_run(&id);
    free_client_id(&id);
    return run_result;
}