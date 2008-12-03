
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
		if (!tapcfg || tapcfg_start(tapcfg, NULL) < 0) {
			printf("Error starting the TAP device, try running as root\n");
			goto exit;
		}
	}

	server = tapserver_init(tapcfg, 50);

	if (!strcmp(argv[1], "client")) {
		struct addrinfo hints, *result, *saddr;
		int sfd;

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
		}
		freeaddrinfo(result);

		if (saddr == NULL) {
			printf("Could not connect to host: %s %s\n",
			       argv[3], argv[4]);
			goto exit;
		}

		tapserver_add_client(server, sfd);
		listen = 0;
	}

	if (tapcfg) {
		srand(time(NULL));
		id = rand()%0x1000;

		sprintf(buffer, "10.10.%d.%d", (id>>8)&0xff, id&0xff);
		printf("Selected IPv4 address: %s\n", buffer);
		tapcfg_iface_set_ipv4(tapcfg, buffer, 16);

		sprintf(buffer, "fc00::%x", id&0xffff);
		printf("Selected IPv6 address: %s\n", buffer);
		tapcfg_iface_add_ipv6(tapcfg, buffer, 64);

		tapcfg_iface_change_status(tapcfg, 1);
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

