
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "tapcfg.h"

#ifdef _WIN32
#  include <ws2tcpip.h>
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netdb.h>
#endif

static tapcfg_t *tapcfg;

void
handle_sigint(int sign) {
	tapcfg_stop(tapcfg);
	tapcfg_destroy(tapcfg);

	exit(0);
}

#ifdef _WIN32
static int
inet_pton(int af, const char *src, void *dst)
{
	struct addrinfo	hints, *res, *ai;
	int ret=0;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = af;

	if (getaddrinfo(src, NULL, &hints, &res) != 0) {
		return -1;
	}

	for (ai = res; ai; ai = ai->ai_next) {
		if (ai->ai_family == af) {
			switch (af) {
			case AF_INET:
			{
				struct sockaddr_in *in;

				in = (struct sockaddr_in *) ai->ai_addr;
				memcpy(dst, &in->sin_addr, sizeof(struct in_addr));
				break;
			}
			case AF_INET6:
			{
				struct sockaddr_in6 *in;
				in = (struct sockaddr_in6 *) ai->ai_addr;
				memcpy(dst, &in->sin6_addr, sizeof(struct in6_addr));
				break;
			}
			default:
				/* Unknown address family, return error */
				ret = -1;
				break;
			}
			break;
		}
	}

	if (!ai) {
		/* No suitable address info structure found */
		ret = -1;
	}
	freeaddrinfo(res);

	return ret;
}
#endif

int
main(int argc, char *argv[])
{
	char buffer[4096+2];
	struct addrinfo hints, *result, *saddr;
	int sfd, id;

	if (argc != 4) {
		printf("Usage: %s -[4|6] <host> <port>\n", argv[0]);
		return -1;
	}

	if (strcmp(argv[1], "-4") &&
	    strcmp(argv[1], "-6")) {
		printf("Unknown IP protocol version: %s\n", argv[1]);
		return -1;
	}

	memset(&hints, 0, sizeof(hints));
	if (!strcmp(argv[1], "-4"))
		hints.ai_family = AF_INET;
	else
		hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo(argv[2], argv[3], &hints, &result)) {
		printf("Unable to resolve host name and port: %s %s\n",
		       argv[2], argv[3]);
		return -1;
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

	if (saddr == NULL) {
		printf("Could not connect to host: %s %s\n",
		       argv[2], argv[3]);
		return -1;
	}
	freeaddrinfo(result);

	srand(time(NULL));
	id = rand()%0x1000;

	tapcfg = tapcfg_init();
	if (!tapcfg)
		goto err;
	if (tapcfg_start(tapcfg, NULL))
		goto err;

	sprintf(buffer, "10.10.%d.%d", (id>>8)&0xff, id&0xff);
	printf("Selected IPv4 address: %s\n", buffer);
	tapcfg_iface_set_ipv4(tapcfg, buffer, 16);

	sprintf(buffer, "fc00::%x", id&0xffff);
	printf("Selected IPv6 address: %s\n", buffer);
	tapcfg_iface_add_ipv6(tapcfg, buffer, 64);

	tapcfg_iface_change_status(tapcfg, 1);

	signal(SIGINT, handle_sigint);
	while(1) {
		fd_set rfds;
		struct timeval tv;
		int ret;

		tv.tv_sec = 0;
		tv.tv_usec = 1000;

		FD_ZERO(&rfds);
		FD_SET(sfd, &rfds);

		select(sfd+1, &rfds, NULL, NULL, &tv);
		if (FD_ISSET(sfd, &rfds)) {
			unsigned char sizebuf[2];
			int size;

			ret = read(sfd, sizebuf, 2);
			if (ret != 2) {
				printf("Error reading size from TCP socket\n");
				exit(-1);
			}

			size = ((int) sizebuf[0]) << 8 | sizebuf[1];
			if (size > sizeof(buffer)) {
				printf("Buffer not big enough for incoming %d "
				       "bytes frame!\n", size);
				exit(-1);
			}

			ret = read(sfd, buffer, size);
			if (ret < 0) {
				printf("Error reading data from TCP socket\n");
				exit(-1);
			}

			printf("Read %d bytes from network\n", ret);

			ret = tapcfg_write(tapcfg, buffer, ret);
			if (ret < 0) {
				printf("Error writing to TAP device\n");
				exit(-1);
			}
		}

		if (tapcfg_can_read(tapcfg)) {
			ret = tapcfg_read(tapcfg, buffer+2, sizeof(buffer)-2);
			if (ret < 0) {
				printf("Error reading from TAP device\n");
				exit(-1);
			}

			printf("Read %d bytes from TAP\n", ret);
			buffer[0] = (ret>>8) & 0xff;
			buffer[1] = ret & 0xff;

			ret = write(sfd, buffer, ret+2);
			if (ret < 0) {
				printf("Error writing to TCP socket\n");
				exit(-1);
			}
		}
	}

	return 0;

err:
	if (tapcfg)
		tapcfg_destroy(tapcfg);
	return -1;
}


