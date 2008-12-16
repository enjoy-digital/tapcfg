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
		HOPOPT  = 0,
		ICMP    = 1,
		IGMP    = 2,
		GGP     = 3,
		IP      = 4,
		ST      = 5,
		TCP     = 6,
		CBT     = 7,
		EGP     = 8,
		IGP     = 9,
		IPv6    = 41,
		ICMPv6  = 58
	}
}
