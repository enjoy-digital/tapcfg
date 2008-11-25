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

#if defined(_WIN32) || defined(_WIN64)

#if (_WIN32_WINNT < 0x0501)
#  define DISABLE_IPV6
#  include <ws2tcpip.h>
#endif

#  include "tapcfg_windows.c"
#else
#  include "tapcfg_unix.c"
#endif

