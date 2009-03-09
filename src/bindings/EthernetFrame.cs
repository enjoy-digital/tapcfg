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

using System;

namespace TAPNet {
	public enum FrameType {
		Ethernet_II,
		Ethernet_RAW,
		Ethernet_IEEE,
		Ethernet_SNAP
	}

	public enum EtherType : int {
		IPv4           = 0x0800,
		ARP            = 0x0806,
		RARP           = 0x8035,
		AppleTalk      = 0x809b,
		AARP           = 0x80f3,
		IPX            = 0x8137,
		IPv6           = 0x86dd,
		CobraNet       = 0x8819
	}

	public class EthernetFrame {
		public readonly FrameType Type;
		public readonly byte[] Data;
		public readonly byte[] Source = new byte[6];
		public readonly byte[] Destination = new byte[6];
		public readonly byte[] Payload;

		/* The VLAN related fields */
		public readonly bool HasVLAN;
		public readonly byte PCP, CFI;
		public readonly int  VID;

		/* The IEEE header related fields */
		public readonly byte DSAP, SSAP, Ctrl;

		/* The OUI field of SNAP header */
		public readonly int OUI;

		private int _etherType;

		public static readonly byte[] Broadcast =
			new byte[] { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

		public EthernetFrame(byte[] data) {
			this.Data = data;
			Array.Copy(data, 0, this.Destination, 0, 6);
			Array.Copy(data, 6, this.Source, 0, 6);
			_etherType = (data[12] << 8) | data[13];
			int hdrlen = 14;

			/* IEEE 802.1Q (VLAN) tagged frame */
			if (_etherType == 0x8100) {
				this.PCP = (byte) ((data[hdrlen] >> 5) & 0x07);
				this.CFI = (byte) ((data[hdrlen] >> 4) & 0x01);
				this.VID = ((data[hdrlen] & 0x0f) << 8) |
				           data[hdrlen+1];
				hdrlen += 2;

				_etherType = (data[hdrlen] << 8) | data[hdrlen+1];
				hdrlen += 2;
			}

			/* This is a common Ethernet II frame */
			if (_etherType >= 0x0800) {
				this.Type = FrameType.Ethernet_II;

				this.Payload = new byte[data.Length - hdrlen];
				Array.Copy(data, hdrlen,
				           this.Payload, 0,
				           this.Payload.Length);
				return;
			}

			/* In IEEE frames etherType is the length */
			int payloadlen = _etherType;

			if (data[hdrlen] == 0xff && data[hdrlen+1] == 0xff) {
				/* Raw Ethernet 802.3 (the broken Novell one)
				 * Always contains a raw IPX frame */
				this.Type = FrameType.Ethernet_RAW;
				_etherType = (int) EtherType.IPX;
			} else {
				/* IEEE 802.2/802.3 Ethernet */
				this.Type = FrameType.Ethernet_IEEE;

				byte DSAP = data[hdrlen++];
				byte SSAP = data[hdrlen++];
//				byte Ctrl = data[hdrlen++];
				payloadlen -= 3;

				if ((DSAP & 0xfe) == 0xaa && (SSAP & 0xfe) == 0xaa) {
					this.Type = FrameType.Ethernet_SNAP;
					OUI = (data[hdrlen + 0] << 8) |
					      (data[hdrlen + 1] << 4) |
					      data[hdrlen + 2];
					payloadlen -= 3;
					hdrlen += 3;

					if (OUI != 0x000000) {
						/* FIXME: Organization Unique Id is
						   non-zero, should mark that */
					}

					_etherType = (data[hdrlen] << 8) | data[hdrlen+1];
					payloadlen -= 2;
					hdrlen += 2;
				}
			}

			/* Copy the final payload data to the payload array */
			this.Payload = new byte[payloadlen];
			Array.Copy(data, hdrlen, this.Payload, 0, this.Payload.Length);
		}

		public EtherType EtherType {
			get { return (EtherType) _etherType; }
		}
	}
}
