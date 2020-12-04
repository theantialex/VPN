#include <stdio.h>

#include "server.h"

int main() {
	hserver_config_t config = {
		.root = "/var/www/",
		.address = "127.0.0.1",
		.port = 80,
		.backlog = 128,
		// .pidfile = "/var/run/hserver/hserver.pid",
	};

	return server_run(&config);
}
