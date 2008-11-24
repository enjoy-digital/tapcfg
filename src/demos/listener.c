
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "compat.h"
#include "listener.h"
#include "taplog.h"
#include "tapcfg.h"

#ifndef TAPCFG_OS_WINDOWS
#  include <pthread.h>
#endif

#define MAX_CLIENTS 5

struct listener_s {
	int server_fd;
	unsigned short port;

	int running;
	tapcfg_t *tapcfg;
#ifndef TAPCFG_OS_WINDOWS
	pthread_t listener_thread;
#else
	HANDLE listener_thread;
#endif
};

listener_t *
listener_init()
{
	listener_t *listener;
#ifdef TAPCFG_OS_WINDOWS
	WORD wVersionRequested;
	WSADATA wsaData;
	int ret;

	wVersionRequested = MAKEWORD(2, 2);

	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret) {
		/* Couldn't find WinSock DLL */
		return NULL;
	}

	if (LOBYTE(wsaData.wVersion) != 2 ||
	    HIBYTE(wsaData.wVersion) != 2) {
		/* Version mismatch, requested version not found */
		return NULL;;
	}
#endif

	listener = calloc(1, sizeof(listener_t));
	if (!listener) {
		return NULL;
	}

	return listener;
}

void
listener_destroy(listener_t *listener)
{
#ifdef TAPCFG_OS_WINDOWS
	WSACleanup();
#endif

	free(listener);
}

unsigned short
listener_get_port(listener_t *listener)
{
	assert(listener);

	return listener->port;
}

void
listener_set_port(listener_t *listener, unsigned short port)
{
	assert(listener);

	listener->port = port;
}

static int
listener_create_server(unsigned short *local_port, int use_ipv6, int public)
{
	int server_fd = -1;
	int socket_domain;
	int ret;

	struct sockaddr *saddr;
	struct sockaddr *caddr;
	socklen_t saddr_size, caddr_size;

	struct sockaddr_in saddr4;
	struct sockaddr_in caddr4;

#ifndef DISABLE_IPV6
	struct sockaddr_in6 saddr6;
	struct sockaddr_in6 caddr6;

	memset(&saddr6, 0, sizeof(saddr6));
	saddr6.sin6_family = AF_INET6;
	saddr6.sin6_addr = (public ? ip6_any : ip6_loopback);
	saddr6.sin6_port = htons(*local_port);

	if (use_ipv6) {
		saddr = (struct sockaddr *) &saddr6;
		saddr_size = sizeof(saddr6);
		caddr = (struct sockaddr *) &caddr6;
		caddr_size = sizeof(caddr6);
		socket_domain = AF_INET6;
	} else
#endif
	{
		saddr = (struct sockaddr *) &saddr4;
		saddr_size = sizeof(saddr4);
		caddr = (struct sockaddr *) &caddr4;
		caddr_size = sizeof(caddr4);
		socket_domain = AF_INET;
	}

	memset(&saddr4, 0, sizeof(saddr4));
	saddr4.sin_family = AF_INET;
	saddr4.sin_addr.s_addr = htonl(public ? INADDR_ANY : INADDR_LOOPBACK);
	saddr4.sin_port = htons(*local_port);

	server_fd = socket(socket_domain, SOCK_STREAM, 0);
	if (server_fd == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error opening socket: %s\n",
		           strerror(errno));
		goto err;
	}

	ret = bind(server_fd, saddr, saddr_size);
	if (ret == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error binding socket: %s\n",
		           strerror(errno));
		goto err;
	}
	getsockname(server_fd, saddr, &saddr_size);

	if (saddr == (struct sockaddr *) &saddr4) {
		*local_port = htons(saddr4.sin_port);
#ifndef DISABLE_IPV6
	} else if (saddr ==  (struct sockaddr *) &saddr6) {
		*local_port = htons(saddr6.sin6_port);
#endif
	}

	if (listen(server_fd, 0) == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error starting to listen socket: %s\n",
		           strerror(errno));
		goto err;
	}

	return server_fd;

err:
	if (server_fd != -1)
		close(server_fd);

	return -1;
}

#ifndef TAPCFG_OS_WINDOWS
static void *
listener_thread(void *arg)
#else
static DWORD WINAPI
listener_thread(LPVOID arg)
#endif
{
	listener_t *listener = arg;
	tapcfg_t *tapcfg = listener->tapcfg;
	unsigned char buf[TAPCFG_MIN_BUFSIZE];
	int clients[MAX_CLIENTS];
	int client_fd;
	int i;

	/* Initialize clients to -1 meaning empty */
	for (i=0; i<MAX_CLIENTS; i++) {
		clients[i] = -1;
	}

	while (listener->running) {
		fd_set rfds;
		struct timeval tv;
		int next_client_idx = -1;
		int highest_fd = -1;
		int retval;

		tv.tv_sec = 0;
		tv.tv_usec = 1000;

		FD_ZERO(&rfds);

		/* Add all connected clients for listening */
		for (i=0; i<MAX_CLIENTS; i++) {
			if (clients[i] != -1) {
				FD_SET(clients[i], &rfds);
				if (clients[i] > highest_fd) {
					highest_fd = clients[i];
				}
			} else {
				/* If client slot is empty, mark available */
				next_client_idx = i;
				break;
			}
		}

		/* Only select server socket if we have available client slots... */
		if (next_client_idx >= 0) {
			FD_SET(listener->server_fd, &rfds);
			if (listener->server_fd > highest_fd) {
				highest_fd = listener->server_fd;
			}
		}

		if (highest_fd < 0) {
			taplog_log(TAPLOG_ERR,
			           "No descriptors to select, shouldn't be possible\n");
			goto cleanup;
		}

		/* Do the actual select operation for all sockets concurrently */
		retval = select(highest_fd+1, &rfds, NULL, NULL, &tv);

		while (tapcfg && tapcfg_has_data(tapcfg)) {
			taplog_log(TAPLOG_INFO, "Reading from device...\n");
			retval = tapcfg_read(tapcfg, buf, sizeof(buf));
			if (retval <= 0) {
				taplog_log(TAPLOG_ERR,
				           "Error reading from TAP device, exiting...\n");
				goto cleanup;
			}

			taplog_log_ethernet_info(buf, retval);

			for (i=0; i<MAX_CLIENTS; i++) {
				unsigned char sizebuf[2];
				int tmp;

				if (clients[i] == -1)
					continue;

				taplog_log(TAPLOG_INFO,
				           "Writing %d bytes to client fd %d\n",
				           retval, clients[i]);

				sizebuf[0] = (retval>>8) & 0xff;
				sizebuf[1] = retval & 0xff;

				/* Write received data for all clients */
				tmp = write(clients[i], sizebuf, 2);
				if (tmp <= 0) {
					clients[i] = -1;
					taplog_log(TAPLOG_DEBUG,
						   "Client %d got disconnected, "
					           "removing from list\n", i);
				}

				/* Write received data for all clients */
				tmp = write(clients[i], buf, retval);
				if (tmp <= 0) {
					clients[i] = -1;
					taplog_log(TAPLOG_DEBUG,
						   "Client %d got disconnected, "
					           "removing from list\n", i);
				}
			}
		}

		for (i=0; i<MAX_CLIENTS; i++) {
			if (clients[i] == -1)
				continue;

			if (FD_ISSET(clients[i], &rfds)) {
				unsigned char sizebuf[2];
				int size;

				retval = read(clients[i], sizebuf, 2);
				if (retval <= 0) {
					clients[i] = -1;
					taplog_log(TAPLOG_DEBUG,
						   "Client %d got disconnected, "
					           "removing from list\n", i);
					continue;
				}

				size = (sizebuf[0]&0xff) << 8 | sizebuf[1];
				if (size > sizeof(buf)) {
					close(clients[i]);
					clients[i] = -1;
					taplog_log(TAPLOG_ERR,
					           "Buffer not big enough for "
					           "incoming %d bytes frame!\n",
					           size);
					continue;
				}

				retval = read(clients[i], buf, size);
				if (retval <= 0) {
					clients[i] = -1;
					taplog_log(TAPLOG_DEBUG,
						   "Client %d got disconnected, "
					           "removing from list\n", i);
					continue;
				}

				taplog_log(TAPLOG_INFO,
					   "Read %d bytes from client fd %d\n",
					   retval, clients[i]);

				taplog_log_ethernet_info(buf, retval);

				if (tapcfg) {
					retval = tapcfg_write(tapcfg, buf, retval);
					if (retval <= 0) {
						taplog_log(TAPLOG_ERR,
							   "Error writing to TAP "
							   "device, exiting...\n");
						goto cleanup;
					}
				} else {
					int j;

					/* If we have no TAP device to write to, forward
					 * the message to all other clients instead... */
					for (j=0; j<MAX_CLIENTS; j++) {
						int tmp;

						if (clients[j] == -1 || i == j)
							continue;

						taplog_log(TAPLOG_INFO,
							   "Writing %d bytes to client fd %d\n",
							   retval, clients[j]);

						/* Write received data for all clients */
						tmp = write(clients[j], sizebuf, 2);
						if (tmp <= 0) {
							clients[j] = -1;
							taplog_log(TAPLOG_DEBUG,
								   "Client %d got disconnected, "
								   "removing from list\n", j);
						}

						/* Write received data for all clients */
						tmp = write(clients[j], buf, retval);
						if (tmp <= 0) {
							clients[j] = -1;
							taplog_log(TAPLOG_DEBUG,
								   "Client %d got disconnected, "
								   "removing from list\n", j);
						}
					}
				}
			}
		}

		/* Accept a client and add it to the client table */
		if (FD_ISSET(listener->server_fd, &rfds)) {
#ifdef DISABLE_IPV6
			struct sockaddr_in caddr;
#else
			struct sockaddr_in6 caddr;
#endif
			socklen_t caddr_size = sizeof(caddr);

			client_fd = accept(listener->server_fd,
			                   (struct sockaddr *) &caddr,
			                   &caddr_size);
			if (client_fd == -1) {
				taplog_log(TAPLOG_ERR,
				           "Error accepting client socket: %s\n",
				           strerror(errno));
				goto cleanup;
			}

			taplog_log(TAPLOG_DEBUG, "Accepted connection %d...\n", client_fd);
			clients[next_client_idx] = client_fd;
		}
	}

cleanup:
	for (i=0; i<MAX_CLIENTS; i++) {
		if (clients[i] >= 0) {
			close(clients[i]);
		}
	}

#ifndef TAPCFG_OS_WINDOWS
	return NULL;
#else
	return 0;
#endif
}

int
listener_start(listener_t *listener, tapcfg_t *tapcfg)
{
	int server_fd;

	assert(listener);

	server_fd = listener_create_server(&listener->port, 1, 1);
	if (server_fd == -1) {
		goto err;
	}
	taplog_log(TAPLOG_DEBUG, "Listening to local port %d\n", listener->port);

	/* Mark the current fds and mark thread as running */
	listener->server_fd = server_fd;
	listener->tapcfg = tapcfg;
	listener->running = 1;

#ifndef TAPCFG_OS_WINDOWS
	if (pthread_create(&listener->listener_thread, NULL, listener_thread, listener))
		listener->listener_thread = 0;
#else
	listener->listener_thread = CreateThread(NULL, 0, listener_thread, listener, 0, NULL);
#endif
	if (!listener->listener_thread) {
		taplog_log(TAPLOG_ERR,
		           "Error creating a listener thread: %s\n",
		           strerror(errno));
		goto err;
	}

	return 0;

err:
	/* Clean up in case of an error */
	if (server_fd != -1)
		close(server_fd);

	listener->listener_thread = 0;
	listener->server_fd = -1;
	listener->running = 0;

	return -1;
}
