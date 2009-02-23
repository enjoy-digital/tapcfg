/**
 *  tapcfg - A cross-platform configuration utility for TAP driver
 *  Copyright (C) 2008-2009  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "daemon.h"
#include "serversock.h"
#include "threads.h"
#include "client.h"

struct daemon_s {
	serversock_t *serversock;
	int server_fd;

	int running;
	mutex_handle_t run_mutex;

	mutex_handle_t mutex;
	thread_handle_t thread;
};

daemon_t *
daemon_init()
{
	daemon_t *daemon;

	daemon = calloc(1, sizeof(daemon_t));
	if (!daemon) {
		return NULL;
	}
	MUTEX_CREATE(daemon->run_mutex);
	MUTEX_CREATE(daemon->mutex);

	return daemon;
}

void
daemon_destroy(daemon_t *daemon)
{
	free(daemon);
}

static THREAD_RETVAL
main_thread(void *arg)
{
	daemon_t *daemon = arg;
	int running;

	assert(daemon);

	printf("Starting daemon listener\n");

	do {
		fd_set rfds;
		struct timeval tv;
		int tmp;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		FD_ZERO(&rfds);
		FD_SET(daemon->server_fd, &rfds);
		tmp = select(daemon->server_fd+1, &rfds, NULL, NULL, &tv);
		if (tmp < 0) {
			printf("Error when selecting for fd\n");
			break;
		}

		if (FD_ISSET(daemon->server_fd, &rfds)) {
			int client_fd;

			printf("Accepting a new client\n");
			client_fd = serversock_accept(daemon->serversock);
			if (client_fd == -1) {
				printf("Error accepting client\n");
				break;
			}
			printf("Accepted a new client\n");
		}

		MUTEX_LOCK(daemon->run_mutex);
		running = daemon->running;
		MUTEX_UNLOCK(daemon->run_mutex);
	} while (running);

	return 0;
}

int
daemon_start(daemon_t *daemon)
{
	unsigned short port = 1234;

	assert(daemon);

	daemon->serversock = serversock_tcp(&port, 0, 1);
	if (!daemon->serversock)
		return -1;
	daemon->server_fd = serversock_get_fd(daemon->serversock);

	daemon->running = 1;
	THREAD_CREATE(daemon->thread, main_thread, daemon);

	return 0;
}

void
daemon_stop(daemon_t *daemon)
{
	assert(daemon);

	MUTEX_LOCK(daemon->run_mutex);
	if (!daemon->running) {
		MUTEX_UNLOCK(daemon->run_mutex);
		return;
	}
	daemon->running = 0;
	MUTEX_UNLOCK(daemon->run_mutex);

	THREAD_JOIN(daemon->thread);

	serversock_destroy(daemon->serversock);
}

