/**
 *  tapcfg - A cross-platform configuration utility for TAP driver
 *  Copyright (C) 2008  Juho Vähä-Herttua
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

#ifndef DISABLE_IPV6
#if defined(_WIN32) || defined(_WIN64)
static const struct in6_addr ip6_any = {{ IN6ADDR_ANY_INIT }};
static const struct in6_addr ip6_loopback = {{ IN6ADDR_LOOPBACK_INIT }};
#else
#define ip6_any in6addr_any
#define ip6_loopback in6addr_loopback
#endif
#endif


static int
create_server(unsigned short *local_port, int use_ipv6, int public)
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
		/* XXX Error opening socket */
		goto err;
	}

	ret = bind(server_fd, saddr, saddr_size);
	if (ret == -1) {
		/* XXX: Error binding socket */
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
		/* XXX Error starting to listen socket */
		goto err;
	}

	return server_fd;

err:
	if (server_fd != -1)
		close(server_fd);

	return -1;
}

