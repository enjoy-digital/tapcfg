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

#ifndef TAPLOG_H
#define TAPLOG_H

/* Define syslog style log levels */
#define TAPLOG_EMERG       0       /* system is unusable */
#define TAPLOG_ALERT       1       /* action must be taken immediately */
#define TAPLOG_CRIT        2       /* critical conditions */
#define TAPLOG_ERR         3       /* error conditions */
#define TAPLOG_WARNING     4       /* warning conditions */
#define TAPLOG_NOTICE      5       /* normal but significant condition */
#define TAPLOG_INFO        6       /* informational */
#define TAPLOG_DEBUG       7       /* debug-level messages */

void taplog_log(int level, const char *fmt, ...);
void taplog_log_ethernet_info(unsigned char *buffer, int len);

#endif /* TAPLOG_H */

