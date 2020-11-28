#include <stdio.h>

#include "server.h"

int main(/*int argc, char *argv[]*/ void) {

	hserver_config_t config = {
		.root = "/var/www/",
		.address = "127.0.0.1",
		.port = 5384,
		.backlog = 128,
		// .pidfile = "/var/run/hserver/hserver.pid",
	};

	return server_run(&config);
}