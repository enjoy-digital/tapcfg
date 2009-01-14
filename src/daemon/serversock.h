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

#ifndef SERVERSOCK_H
#define SERVERSOCK_H

#include "tapcfg.h"

typedef struct serversock_s serversock_t;

serversock_t *serversock_tcp(unsigned short *local_port, int use_ipv6, int public);
int serversock_get_fd(serversock_t *server);
int serversock_accept(serversock_t *server);
void serversock_destroy(serversock_t *server);

#endif
