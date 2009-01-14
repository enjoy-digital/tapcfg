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

#include "daemon.h"
#include "threads.h"

struct daemon_s {
};

daemon_t *
daemon_init()
{
	daemon_t *daemon;

	daemon = malloc(sizeof(daemon_t));
	if (!daemon) {
		return NULL;
	}

	return daemon;
}

void
daemon_destroy(daemon_t *daemon)
{
	free(daemon);
}

int
daemon_start(daemon_t *daemon)
{
	return 0;
}

void
daemon_stop(daemon_t *daemon)
{
}
