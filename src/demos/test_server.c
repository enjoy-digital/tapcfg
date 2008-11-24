
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "compat.h"
#include "tapcfg.h"
#include "listener.h"

static tapcfg_t *tapcfg;

void
handle_sigint(int sign) {
	tapcfg_stop(tapcfg);
	tapcfg_destroy(tapcfg);

	exit(0);
}

int
main(int argc, char *argv[])
{
	struct in_addr ipv4addr;
	struct in6_addr ipv6addr;
	char buffer[256];
	listener_t *listener;
	int id;

	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		return -1;
	}

	srand(time(NULL));
	id = rand()%0x1000;

	sprintf(buffer, "10.10.%d.%d", (id>>8)&0xff, id&0xff);
	printf("Selected IPv4 address: %s\n", buffer);
	inet_pton(AF_INET, buffer, &ipv4addr);
	sprintf(buffer, "fc00::%x", id&0xffff);
	printf("Selected IPv6 address: %s\n", buffer);
	inet_pton(AF_INET6, buffer, &ipv6addr);

	tapcfg = tapcfg_init();
	if (!tapcfg)
		goto err;
	if (tapcfg_start(tapcfg))
		goto err;

	tapcfg_iface_set_ipv4(tapcfg, &ipv4addr, 24);
	tapcfg_iface_add_ipv6(tapcfg, &ipv6addr, 64);

	listener = listener_init();
	if (!listener)
		goto err;

	tapcfg_iface_change_status(tapcfg, 1);
	listener_set_port(listener, atoi(argv[1]));
	listener_start(listener, tapcfg);

	signal(SIGINT, handle_sigint);
	while(1) {
		sleep(1);
	}

	return 0;

err:
	if (listener)
		listener_destroy(listener);
	if (tapcfg)
		tapcfg_destroy(tapcfg);

	return -1;
}


