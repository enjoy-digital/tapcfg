/**
 *  tapcfg - A cross-platform configuration utility for TAP driver
 *  Copyright (C) 2008-2011  Juho Vähä-Herttua
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
		public int EtherType;
		public readonly byte[] Payload;

		/* The 802.1QinQ related fields */
		public readonly bool HasQinQ;
		public readonly int  QinQType;
		public readonly int  QinQValue;

		/* The 802.1Q (VLAN) related fields */
		public readonly bool HasVLAN;
		public readonly byte PCP;
		public readonly bool CFI;
		public readonly int  VID;

		/* The IEEE header related fields */
		public readonly byte DSAP, SSAP, Ctrl;

		/* The SNAP header related OUI field */
		public readonly int OUI;


		public static readonly byte[] Broadcast =
			new byte[] { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

		public EthernetFrame(byte[] data) {
			this.Data = (byte[]) data.Clone();
			Array.Copy(data, 0, this.Destination, 0, 6);
			Array.Copy(data, 6, this.Source, 0, 6);
			this.EtherType = (data[12] << 8) | data[13];
			int hdrlen = 14;

			/* IEEE 802.1QinQ (VLAN) tagged frame*/
			switch (this.EtherType) {
			case 0x8100: // IEEE 802.1Q (VLAN)
			case 0x88a8: // 802.1ad
			case 0x9100: // Unknown 802.1QinQ
			case 0x9200: // Unknown 802.1QinQ
				this.HasQinQ = true;
				this.QinQType = this.EtherType;
				this.QinQValue = (data[hdrlen] << 8) | data[hdrlen+1];
				hdrlen += 2;

				this.EtherType = (data[hdrlen] << 8) | data[hdrlen+1];
				hdrlen += 2;
				break;
			}

			/* IEEE 802.1Q (VLAN) tagged frame detection */
			int vlanData = 0;
			if (this.EtherType == 0x8100) {
				this.HasVLAN = true;
				vlanData = (data[hdrlen] << 8) | data[hdrlen+1];
				hdrlen += 2;

				this.EtherType = (data[hdrlen] << 8) | data[hdrlen+1];
				hdrlen += 2;
			} else if (this.HasQinQ && this.QinQType == 0x8100) {
				this.HasVLAN = true;
				vlanData = this.QinQValue;

				this.HasQinQ = false;
				this.QinQType = 0;
				this.QinQValue = 0;
			} else if (this.HasQinQ) {
				// QinQ detected incorrectly, reset to defaults
				this.HasQinQ = false;
				this.QinQType = 0;
				this.QinQValue = 0;
				this.EtherType = (data[12] << 8) | data[13];
				hdrlen = 14;
			}

			/* IEEE 802.1Q (VLAN) tagged frame parsing */
			if (this.HasVLAN) {
				this.PCP = (byte) ((vlanData >> 13) & 0x07);
				this.CFI = ((vlanData >> 12) & 0x01) != 0;
				this.VID = vlanData & 0xfff;
			}

			/* This is a common Ethernet II frame */
			if (this.EtherType >= 0x0800) {
				this.Type = FrameType.Ethernet_II;

				this.Payload = new byte[data.Length - hdrlen];
				Array.Copy(data, hdrlen,
				           this.Payload, 0,
				           this.Payload.Length);
				return;
			}

			/* In IEEE frames etherType is the length */
			int payloadlen = this.EtherType;

			if (data[hdrlen] == 0xff && data[hdrlen+1] == 0xff) {
				/* Raw Ethernet 802.3 (the broken Novell one)
				 * Always contains a raw IPX frame */
				this.Type = FrameType.Ethernet_RAW;
				this.EtherType = 0x8137; /* The type of IPX */
			} else {
				/* IEEE 802.2/802.3 Ethernet */
				this.Type = FrameType.Ethernet_IEEE;

				this.DSAP = data[hdrlen++];
				this.SSAP = data[hdrlen++];
				this.Ctrl = data[hdrlen++];
				payloadlen -= 3;

				if ((DSAP & 0xfe) == 0xaa && (SSAP & 0xfe) == 0xaa) {
					this.Type = FrameType.Ethernet_SNAP;
					this.OUI = (data[hdrlen + 0] << 8) |
					           (data[hdrlen + 1] << 4) |
					           data[hdrlen + 2];
					payloadlen -= 3;
					hdrlen += 3;

					this.EtherType = (data[hdrlen] << 8) | data[hdrlen+1];
					payloadlen -= 2;
					hdrlen += 2;
				}
			}

			/* Copy the final payload data to the payload array */
			this.Payload = new byte[payloadlen];
			Array.Copy(data, hdrlen, this.Payload, 0, this.Payload.Length);
		}
	}
}
