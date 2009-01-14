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

#ifndef TAPSERVER_H
#define TAPSERVER_H

#include "tapcfg.h"

typedef struct tapserver_s tapserver_t;

tapserver_t *tapserver_init(tapcfg_t *tapcfg, int waitms);
void tapserver_destroy(tapserver_t *server);
int tapserver_add_client(tapserver_t *server, int fd);
int tapserver_start(tapserver_t *server, unsigned short port, int listen);
void tapserver_stop(tapserver_t *server);


#endif
