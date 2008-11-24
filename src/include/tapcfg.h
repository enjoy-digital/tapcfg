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

#ifndef TAPCFG_H
#define TAPCFG_H

#include "compat.h"

#define TAPCFG_MIN_BUFSIZE 4096

typedef struct tapcfg_s tapcfg_t;

tapcfg_t *tapcfg_init();
void tapcfg_destroy(tapcfg_t *tapcfg);

int tapcfg_start(tapcfg_t *tapcfg);
void tapcfg_stop(tapcfg_t *tapcfg);

int tapcfg_has_data(tapcfg_t *tapcfg);
int tapcfg_read(tapcfg_t *tapcfg, void *buf, int count);
int tapcfg_write(tapcfg_t *tapcfg, void *buf, int count);

const char *tapcfg_get_ifname(tapcfg_t *tapcfg);

int tapcfg_iface_get_status(tapcfg_t *tapcfg);
int tapcfg_iface_change_status(tapcfg_t *tapcfg, int enabled);
int tapcfg_iface_set_ipv4(tapcfg_t *tapcfg, struct in_addr *addr, unsigned char netbits);
int tapcfg_iface_add_ipv6(tapcfg_t *tapcfg, struct in6_addr *addr, unsigned char netbits);

#endif /* TAPCFG_H */

