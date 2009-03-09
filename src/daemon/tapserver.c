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
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <netinet/in.h>
#  include <sys/types.h>
#  include <sys/socket.h>
#endif

#include "tapserver.h"
#include "serversock.h"
#include "threads.h"

#define MAX_CLIENTS 5

struct tapserver_s {
	serversock_t *serversock;
	int server_fd;
	unsigned short server_port;

	int running;
	int joined;
	mutex_handle_t run_mutex;

	int listening;
	int max_clients;
	tapcfg_t *tapcfg;
	int waitms;

	int clients;
	int clienttab[MAX_CLIENTS];
	mutex_handle_t mutex;

	thread_handle_t reader;
	thread_handle_t writer;
};


tapserver_t *
tapserver_init(tapcfg_t *tapcfg, int waitms)
{
	tapserver_t *server;

	server = calloc(1, sizeof(tapserver_t));
	if (!server) {
		return NULL;
	}
	server->max_clients = MAX_CLIENTS;
	server->tapcfg = tapcfg;
	server->waitms = waitms;
	MUTEX_CREATE(server->run_mutex);
	MUTEX_CREATE(server->mutex);

	return server;
}

void
tapserver_destroy(tapserver_t *server)
{
	if (server) {
		MUTEX_DESTROY(server->mutex);
		MUTEX_DESTROY(server->run_mutex);
	}
	free(server);
}

int
tapserver_add_client(tapserver_t *server, int fd)
{
	assert(server);

	MUTEX_LOCK(server->mutex);
	if (server->clients >= server->max_clients) {
		MUTEX_UNLOCK(server->mutex);
		return -1;
	}
	server->clienttab[server->clients] = fd;
	server->clients++;
	MUTEX_UNLOCK(server->mutex);

	return 0;
}

static void
remove_client(tapserver_t *server, int idx)
{
	assert(server);
	assert(idx < server->clients);

	if (idx < (server->clients-1)) {
		memmove(&server->clienttab[idx],
			&server->clienttab[idx+1],
			sizeof(int) *
			(server->clients - idx - 1));
	}
	server->clients--;
}

static int
send_data(int s, void *buf, int len)
{
	int sent = 0;

	while (len > sent) {
		int ret = send(s, buf+sent, len-sent, 0);
		if (ret == -1)
			return -1;
		sent += ret;
	}

	return sent;
}

static int
recv_data(int s, void *buf, int len)
{
	int recvd = 0;

	while (len > recvd) {
		int ret = recv(s, buf+recvd, len-recvd, 0);
		if (ret == -1)
			return -1;
		recvd += ret;
	}

	return recvd;
}

static THREAD_RETVAL
reader_thread(void *arg)
{
	tapserver_t *server = arg;
	tapcfg_t *tapcfg = server->tapcfg;
	unsigned char buf[4096];
	int running;
	int i, tmp;

	assert(server);

	/* If we don't have tapcfg, finish the thread */
	if (!tapcfg) {
		return 0;
	}

	printf("Starting reader thread\n");

	do {
		while (tapcfg_wait_readable(tapcfg, server->waitms)) {
			int len;

			len = tapcfg_read(tapcfg, buf, sizeof(buf));
			if (len <= 0) {
				/* XXX: We could quite more nicely */
				break;
			}
			printf("Read %d bytes from the device\n", len);

			MUTEX_LOCK(server->mutex);
			for (i=0; i<server->clients; i++) { 
				unsigned char sizebuf[2];

				sizebuf[0] = (len >> 8) & 0xff;
				sizebuf[1] = len & 0xff;

				/* Write received data length */
				tmp = send_data(server->clienttab[i], sizebuf, 2);
				if (tmp > 0) {
					/* Write received data */
					tmp = send_data(server->clienttab[i], buf, len);
				}

				if (tmp <= 0) {
					remove_client(server, i);
				}
				printf("Wrote %d bytes to the client\n", len);
			}
			MUTEX_UNLOCK(server->mutex);
		}

		MUTEX_LOCK(server->run_mutex);
		running = server->running;
		MUTEX_UNLOCK(server->run_mutex);
	} while (running);

	printf("Stopping reader thread\n");

	return 0;
}

static THREAD_RETVAL
writer_thread(void *arg)
{
	tapserver_t *server = arg;
	tapcfg_t *tapcfg = server->tapcfg;
	unsigned char buf[4096];
	int running;
	int i, j, tmp;

	assert(server);

	printf("Starting writer thread\n");

	do {
		fd_set rfds;
		struct timeval tv;
		int highest_fd = -1;

		FD_ZERO(&rfds);

		MUTEX_LOCK(server->mutex);
		if (server->listening && server->clients < server->max_clients) {
			FD_SET(server->server_fd, &rfds);
			highest_fd = server->server_fd;
		}
		for (i=0; i<server->clients; i++) {
			FD_SET(server->clienttab[i], &rfds);
			if (server->clienttab[i] > highest_fd) {
				highest_fd = server->clienttab[i];
			}
		}
		MUTEX_UNLOCK(server->mutex);

		if (highest_fd == -1) {
			break;
		}

		tv.tv_sec = server->waitms / 1000;
		tv.tv_usec = (server->waitms % 1000) * 1000;
		tmp = select(highest_fd+1, &rfds, NULL, NULL, &tv);
		if (tmp < 0) {
			printf("Error when selecting for fds\n");
			break;
		}

		MUTEX_LOCK(server->mutex);
		for (i=0; i<server->clients; i++) {
			unsigned char sizebuf[2];
			int len;

			if (!FD_ISSET(server->clienttab[i], &rfds))
				continue;

			tmp = recv_data(server->clienttab[i], sizebuf, 2);
			if (tmp > 0) {
				len = (sizebuf[0]&0xff) << 8 | sizebuf[1];
				if (len <= sizeof(buf)) {
					tmp = recv_data(server->clienttab[i], buf, len);
				} else {
					/* XXX: Buffer size error handled as read error */
					tmp = -1;
				}
			}
			if (tmp <= 0) {
				remove_client(server, i);
				continue;
			}
			printf("Read %d bytes from the client\n", len);

			if (tapcfg) {
				tmp = tapcfg_write(tapcfg, buf, len);
				if (tmp <= 0) {
					MUTEX_LOCK(server->run_mutex);
					server->running = 0;
					MUTEX_UNLOCK(server->run_mutex);
					MUTEX_UNLOCK(server->mutex);
					goto exit;
				}
				printf("Wrote %d bytes to the device\n", len);
			} else {
				for (j=0; j<server->clients; j++) {
					if (i == j) {
						continue;
					}

					tmp = send_data(server->clienttab[j], sizebuf, 2);
					if (tmp > 0) {
						tmp = send_data(server->clienttab[j], buf, len);
					}
					if (tmp <= 0) {
						remove_client(server, j);
					}
					printf("Wrote %d bytes to the client\n", len);
				}
			}
		}
		MUTEX_UNLOCK(server->mutex);

		/* Accept a client and add it to the client table */
		if (server->listening && FD_ISSET(server->server_fd, &rfds)) {
			int client_fd;

			printf("Accepting a new client\n");
			client_fd = serversock_accept(server->serversock);
			if (client_fd == -1) {
				/* XXX: This error should definitely be reported */
				goto exit;
			}
			printf("Accepted a new client\n");

			tapserver_add_client(server, client_fd);
		}

		MUTEX_LOCK(server->run_mutex);
		running = server->running;
		MUTEX_UNLOCK(server->run_mutex);
	} while (running);

exit:
	printf("Stopping writer thread\n");

	return 0;
}

int
tapserver_start(tapserver_t *server, unsigned short port, int listen)
{
	assert(server);

	if (listen) {
		server->serversock = serversock_tcp(&port, 0, 1);
		if (!server->serversock)
			return -1;

		server->server_fd = serversock_get_fd(server->serversock);
		server->listening = 1;
	} else {
		server->listening = 0;
	}
	server->running = 1;
	server->joined = 0;

	THREAD_CREATE(server->reader, reader_thread, server);
	THREAD_CREATE(server->writer, writer_thread, server);

	return 0;
}

void
tapserver_stop(tapserver_t *server)
{
	assert(server);

	MUTEX_LOCK(server->run_mutex);
	if (server->joined) {
		MUTEX_UNLOCK(server->run_mutex);
		return;
	}
	server->running = 0;
	server->joined = 1;
	MUTEX_UNLOCK(server->run_mutex);

	THREAD_JOIN(server->reader);
	THREAD_JOIN(server->writer);

	serversock_destroy(server->serversock);
}

int
tapserver_client_count(tapserver_t *server)
{
	int ret;

	assert(server);

	MUTEX_LOCK(server->mutex);
	ret = server->clients;
	MUTEX_UNLOCK(server->mutex);

	return ret;
}
