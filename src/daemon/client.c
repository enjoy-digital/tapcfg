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
#include <assert.h>

#include "client.h"
#include "daemon.h"
#include "threads.h"

struct client_s {
	daemon_t *daemon;
	int fd;

	int running;
	mutex_handle_t run_mutex;

	thread_handle_t thread;
};

client_t *
client_init(daemon_t *daemon, int fd)
{
	client_t *client;

	assert(daemon);

	client = malloc(sizeof(client_t));
	if (!client) {
		return NULL;
	}

	client->daemon = daemon;
	client->fd = fd;
	MUTEX_CREATE(client->run_mutex);

	return client;
}

void
client_destroy(client_t *client)
{
	free(client);
}

static THREAD_RETVAL
client_thread(void *arg)
{
	client_t *client = arg;
	int running;

	assert(client);

	do {

		MUTEX_LOCK(client->run_mutex);
		running = client->running;
		MUTEX_UNLOCK(client->run_mutex);
	} while (running);

	return 0;
}

void
client_start(client_t *client)
{
	assert(client);

	client->running = 1;
	THREAD_CREATE(client->thread, client_thread, client);
}

