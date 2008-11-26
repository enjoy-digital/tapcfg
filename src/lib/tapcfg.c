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

#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#  include <ws2tcpip.h>
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
#endif

#include "tapcfg.h"
#include "taplog.h"

#define TAPCFG_BUFSIZE 2048

static int tapcfg_address_is_valid(int family, char *addr) {
	struct addrinfo hints, *res;

	/* Try to convert the address string into a structure */
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_family = family;
	if (getaddrinfo(addr, NULL, &hints, &res)) {
		taplog_log(TAPLOG_ERR,
		           "Error converting string '%s' to "
		           "address, check the format\n", addr);
		return 0;
	}
	freeaddrinfo(res);

	return 1;
}

#if defined(_WIN32) || defined(_WIN64)
#  include "tapcfg_windows.c"
#else
#  include "tapcfg_unix.c"
#endif

