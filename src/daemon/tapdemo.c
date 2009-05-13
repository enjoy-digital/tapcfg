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
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include "tapserver.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

static int running = 0;

void
handle_sigint(int sign)
{
	running = 0;
}

static void usage(char *prog)
{
	printf("Usage of the program:\n");
	printf("    %s server <port>\n", prog);
	printf("    %s client [-4|-6] <host> <port>\n", prog);
	printf("    %s forwarder <port>\n", prog);
}

int main(int argc, char *argv[]) {
	tapcfg_t *tapcfg = NULL;
	tapserver_t *server = NULL;
	unsigned short port = 0;
	char buffer[256];
	int listen = 1;
	int id;

#ifdef _WIN32
#define sleep(x) Sleep((x)*1000)

	WORD wVersionRequested;
	WSADATA wsaData;
	int ret;

	wVersionRequested = MAKEWORD(2, 2);

	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret) {
		/* Couldn't find WinSock DLL */
		return -1;
	}

	if (LOBYTE(wsaData.wVersion) != 2 ||
	    HIBYTE(wsaData.wVersion) != 2) {
		/* Version mismatch, requested version not found */
		return -1;
	}
#endif
	if (argc < 2 ||
	    (!strcmp(argv[1], "server") && argc < 3) ||
	    (!strcmp(argv[1], "client") && argc < 5) ||
	    (!strcmp(argv[1], "forwarder") && argc < 3)) {
		printf("Too few arguments for the application\n");
		usage(argv[0]);
		return -1;
	}

	if (strcmp(argv[1], "server") &&
	    strcmp(argv[1], "client") &&
	    strcmp(argv[1], "forwarder")) {
		printf("Invalid command: \"%s\"\n", argv[1]);
		usage(argv[0]);
		return -1;
	}

	if (!strcmp(argv[1], "server") ||
	    !strcmp(argv[1], "forwarder")) {
		port = atoi(argv[2]);
	}

	if (!strcmp(argv[1], "server") || !strcmp(argv[1], "client")) {
		tapcfg = tapcfg_init();
		if (!tapcfg || tapcfg_start(tapcfg, NULL, 1) < 0) {
			printf("Error starting the TAP device, try running as root\n");
			goto exit;
		}
	}

	server = tapserver_init(tapcfg, 50);

	if (!strcmp(argv[1], "client")) {
		int sfd = -1;
#ifdef HAVE_GETADDRINFO
		struct addrinfo hints, *result, *saddr;

		memset(&hints, 0, sizeof(hints));
		if (!strcmp(argv[2], "-4"))
			hints.ai_family = AF_INET;
		else
			hints.ai_family = AF_INET6;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = 0;
		hints.ai_protocol = IPPROTO_TCP;

		if (getaddrinfo(argv[3], argv[4], &hints, &result)) {
			printf("Unable to resolve host name and port: %s %s\n",
				argv[3], argv[4]);
			goto exit;
		}

		for (saddr = result; saddr != NULL; saddr = saddr->ai_next) {
			sfd = socket(saddr->ai_family, saddr->ai_socktype,
			             saddr->ai_protocol);
			if (sfd == -1)
				continue;

			if (connect(sfd, saddr->ai_addr, saddr->ai_addrlen) != -1)
				break;

			close(sfd);
			sfd = -1;
		}
		freeaddrinfo(result);
#else
		struct sockaddr_in saddr;

		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = inet_addr(argv[3]);
		saddr.sin_port = atoi(argv[4]); 

		sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sfd != -1) {
			if (connect(sfd, (struct sockaddr *) &saddr, sizeof(saddr)) == -1) {
				close(sfd);
				sfd = -1;
			}
		}
#endif

		if (sfd == -1) {
			printf("Could not connect to host: %s %s\n",
			       argv[3], argv[4]);
			goto exit;
		}

		tapserver_add_client(server, sfd);
		listen = 0;
	}

	if (tapcfg) {
		const char *hwaddr;
		char *ifname;
		int hwaddrlen;
		int i, ret;

		hwaddr = tapcfg_iface_get_hwaddr(tapcfg, &hwaddrlen);
		printf("Got hardware address: ");
		for (i=0; i<hwaddrlen; i++) {
			printf("%.2x%s",
			       (unsigned char) hwaddr[i],
			       (i == hwaddrlen-1) ? "\n" : ":");
		}

		ifname = tapcfg_get_ifname(tapcfg);
		printf("Got ifname: %s\n", ifname);
		free(ifname);

		srand(time(NULL));
		id = rand()%0x1000;

		sprintf(buffer, "10.10.%d.%d", (id>>8)&0xff, id&0xff);
		ret = tapcfg_iface_set_ipv4(tapcfg, buffer, 16);
		if (ret == -1) {
			printf("Error setting IPv4 address\n");
		} else {
			printf("Selected IPv4 address: %s\n", buffer);
		}

		ret = tapcfg_iface_get_mtu(tapcfg);
		printf("Old MTU is %d\n", ret);
		if (tapcfg_iface_set_mtu(tapcfg, 1280) == -1) {
			printf("Error setting the new MTU\n");
		}
		ret = tapcfg_iface_get_mtu(tapcfg);
		printf("New MTU is %d\n", ret);

		ret = tapcfg_iface_set_status(tapcfg,
			TAPCFG_STATUS_IPV4_UP | TAPCFG_STATUS_IPV6_UP);
		if (ret == -1) {
			printf("Error changing interface status\n");
		}
	}

	if (tapserver_start(server, port, listen) < 0) {
		printf("Error starting the tapserver\n");
		goto exit;
	}

	running = 1;
	signal(SIGINT, handle_sigint);
	while (running) {
		sleep(1);
	}

exit:
	if (server) {
		tapserver_stop(server);
		tapserver_destroy(server);
	}
	if (tapcfg) {
		tapcfg_destroy(tapcfg);
	}

#ifdef _WIN32
	WSACleanup();
#endif

	return 0;
}

