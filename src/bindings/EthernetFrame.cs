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

using System;

namespace TAP {
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
		private FrameType frameType;
		private byte[] data;
		private byte[] src = new byte[6];
		private byte[] dst = new byte[6];
		private int etherType;
		private byte[] payload;

		public EthernetFrame(byte[] data) {
			parseData(data);
		}

		private void parseData(byte[] data) {
			this.data = data;
			Array.Copy(data, 0, dst, 0, 6);
			Array.Copy(data, 6, src, 0, 6);
			etherType = (data[12] << 8) | data[13];
			int hdrlen = 14;

			/* IEEE 802.1Q tagged frame */
			if (etherType == 0x8100) {
//				int PCP = (data[hdrlen] >> 5) & 0x07;
//				int CFI = (data[hdrlen] >> 4) & 0x01;
//				int VID = ((data[hdrlen] & 0x0f) << 8) |
//				          data[hdrlen+1];
				hdrlen += 2;

				etherType = (data[hdrlen] << 8) | data[hdrlen+1];
				hdrlen += 2;
			}

			/* This is a common Ethernet II frame */
			if (etherType >= 0x0800) {
				frameType = FrameType.Ethernet_II;

				payload = new byte[data.Length - hdrlen];
				Array.Copy(data, hdrlen,
				           payload, 0,
				           payload.Length);
				return;
			}

			/* In IEEE frames etherType is the length */
			int payloadlen = etherType;

			if (data[hdrlen] == 0xff && data[hdrlen+1] == 0xff) {
				/* Raw Ethernet 802.3 (the broken Novell one)
				 * Always contains a raw IPX frame */
				frameType = FrameType.Ethernet_RAW;
				etherType = (int) EtherType.IPX;
			} else {
				/* IEEE 802.2/802.3 Ethernet */
				frameType = FrameType.Ethernet_IEEE;

				byte DSAP = data[hdrlen++];
				byte SSAP = data[hdrlen++];
//				byte Ctrl = data[hdrlen++];
				payloadlen -= 3;

				if ((DSAP & 0xfe) == 0xaa && (SSAP & 0xfe) == 0xaa) {
					frameType = FrameType.Ethernet_SNAP;
					int OUI = (data[hdrlen + 0] << 8) |
					          (data[hdrlen + 1] << 4) |
					           data[hdrlen + 2];
					payloadlen -= 3;
					hdrlen += 3;

					if (OUI != 0x000000) {
						/* FIXME: Organization Unique Id is
						   non-zero, should mark that */
					}

					etherType = (data[hdrlen] << 8) | data[hdrlen+1];
					payloadlen -= 2;
					hdrlen += 2;
				}
			}

			/* Copy the final payload data to the payload array */
			payload = new byte[payloadlen];
			Array.Copy(data, hdrlen, payload, 0, payload.Length);
		}

		public byte[] Data {
			get { return data; }
		}

		public FrameType Type {
			get { return frameType; }
		}

		public byte[] SourceAddress {
			get { return src; }
		}

		public byte[] DestinationAddress {
			get { return dst; }
		}

		public EtherType EtherType {
			get { return (EtherType) etherType; }
		}

		public byte[] Payload {
			get { return payload; }
		}
	}
}
