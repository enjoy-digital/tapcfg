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

#ifndef COMPAT_H
#define COMPAT_H

#if defined(__linux__)
#  define TAPCFG_OS_LINUX
#elif defined(_WIN32) || defined(_WIN64)
#  define TAPCFG_OS_WINDOWS
#elif defined(__darwin__)
#  define TAPCFG_OS_DARWIN
#elif defined(__BSD__)
#  define TAPCFG_OS_BSD
#endif


#ifdef TAPCFG_OS_WINDOWS

/* IPv6 is disabled for earlier than Windows XP releases */
#if (_WIN32_WINNT < 0x0501)
#define DISABLE_IPV6
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#define sleep(x) Sleep((x)*1000)

#ifndef DISABLE_IPV6
static const struct in6_addr ip6_any = {{ IN6ADDR_ANY_INIT }};
static const struct in6_addr ip6_loopback = {{ IN6ADDR_LOOPBACK_INIT }};
#endif

#else

#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define ip6_any in6addr_any
#define ip6_loopback in6addr_loopback

#endif

#endif /* COMPAT_H */
