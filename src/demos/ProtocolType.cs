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

namespace TAP {
	/* http://www.iana.org/assignments/protocol-numbers/ */
	public enum ProtocolType : byte {
		HOPOPT  = 1,
		ICMP    = 2,
		IGMP    = 3,
		GGP     = 4,
		IP      = 5,
		IPv6    = 41,
		ICMPv6  = 58
	}
}
