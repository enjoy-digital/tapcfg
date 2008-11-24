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

#include "compat.h"


#ifdef TAPCFG_OS_WINDOWS

const char *
inet_ntop(int af, const void *src, char *dst, socklen_t cnt)
{
	char *ret = NULL;

	if (af == AF_INET) {
		struct sockaddr_in in;

		memset(&in, 0, sizeof(in));
		in.sin_family = AF_INET;
		memcpy(&in.sin_addr, src, sizeof(struct in_addr));
		if (!getnameinfo((struct sockaddr *) &in, sizeof(in),
		                 dst, cnt, NULL, 0, NI_NUMERICHOST)) {
			ret = dst;
		}
	}
#ifndef DISABLE_IPV6
	else if (af == AF_INET6) {
		struct sockaddr_in6 in;

		memset(&in, 0, sizeof(in));
		in.sin6_family = AF_INET6;
		memcpy(&in.sin6_addr, src, sizeof(struct in6_addr));
		if (!getnameinfo((struct sockaddr *) &in, sizeof(in),
		                 dst, cnt, NULL, 0, NI_NUMERICHOST)) {
			ret = dst;
		}
	}
#endif

	return ret;
}

#ifdef DISABLE_IPV6
int
inet_pton(int af, const char *src, void *dst)
{
	struct in_addr *address = dst;

	if (af == AF_INET) {
		address->s_addr = inet_addr(src);
		return 0;
	}

	/* Unknown address family, return error */
	return -1;
}
#else
int
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
#endif /* DISABLE_IPV6 */

#endif /* TAPCFG_OS_WINDOWS */
