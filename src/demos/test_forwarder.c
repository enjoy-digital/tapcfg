
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "compat.h"
#include "listener.h"

void
handle_sigint(int sign) {
	exit(0);
}

int
main(int argc, char *argv[])
{
	listener_t *listener;

	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		return -1;
	}

	listener = listener_init();
	if (!listener)
		return -1;

	listener_set_port(listener, atoi(argv[1]));
	listener_start(listener, NULL);

	signal(SIGINT, handle_sigint);
	while(1) {
		sleep(1);
	}

	return 0;
}


