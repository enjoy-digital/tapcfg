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
		private FrameType _frameType;
		private byte[] _data;
		private byte[] _src = new byte[6];
		private byte[] _dst = new byte[6];
		private int _etherType;
		private byte[] _payload;

		private EthernetFrame() {
		}

		public EthernetFrame(byte[] data) {
			parseData(data);
		}

		private void parseData(byte[] data) {
			this._data = data;
			Array.Copy(data, 0, _dst, 0, 6);
			Array.Copy(data, 6, _src, 0, 6);
			_etherType = (data[12] << 8) | data[13];
			int hdrlen = 14;

			/* IEEE 802.1Q tagged frame */
			if (_etherType == 0x8100) {
//				int PCP = (data[hdrlen] >> 5) & 0x07;
//				int CFI = (data[hdrlen] >> 4) & 0x01;
//				int VID = ((data[hdrlen] & 0x0f) << 8) |
//				          data[hdrlen+1];
				hdrlen += 2;

				_etherType = (data[hdrlen] << 8) | data[hdrlen+1];
				hdrlen += 2;
			}

			/* This is a common Ethernet II frame */
			if (_etherType >= 0x0800) {
				_frameType = FrameType.Ethernet_II;

				_payload = new byte[data.Length - hdrlen];
				Array.Copy(data, hdrlen,
				           _payload, 0,
				           _payload.Length);
				return;
			}

			/* In IEEE frames etherType is the length */
			int payloadlen = _etherType;

			if (data[hdrlen] == 0xff && data[hdrlen+1] == 0xff) {
				/* Raw Ethernet 802.3 (the broken Novell one)
				 * Always contains a raw IPX frame */
				_frameType = FrameType.Ethernet_RAW;
				_etherType = (int) EtherType.IPX;
			} else {
				/* IEEE 802.2/802.3 Ethernet */
				_frameType = FrameType.Ethernet_IEEE;

				byte DSAP = data[hdrlen++];
				byte SSAP = data[hdrlen++];
//				byte Ctrl = data[hdrlen++];
				payloadlen -= 3;

				if ((DSAP & 0xfe) == 0xaa && (SSAP & 0xfe) == 0xaa) {
					_frameType = FrameType.Ethernet_SNAP;
					int OUI = (data[hdrlen + 0] << 8) |
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
			_payload = new byte[payloadlen];
			Array.Copy(data, hdrlen, _payload, 0, _payload.Length);
		}

		public byte[] Data {
			get { return _data; }
		}

		public FrameType Type {
			get { return _frameType; }
		}

		public byte[] SourceAddress {
			get { return _src; }
		}

		public byte[] DestinationAddress {
			get { return _dst; }
		}

		public EtherType EtherType {
			get { return (EtherType) _etherType; }
		}

		public byte[] Payload {
			get { return _payload; }
		}

		public static EthernetFrame CreateFrame(EtherType type,
		                                        byte[] source,
		                                        byte[] destination,
		                                        byte[] payload) {
			if (source.Length != 6 || destination.Length != 6) {
				throw new Exception("Invalid address length");
			}

			EthernetFrame ret = new EthernetFrame();
			ret._frameType = FrameType.Ethernet_II;
			ret._src = source;
			ret._dst = destination;
			ret._etherType = (int) type;
			ret._payload = payload;

			ret._data = new byte[18];
			Array.Copy(ret._dst, 0, ret._data, 0, 6);
			Array.Copy(ret._src, 0, ret._data, 6, 6);
			ret._data[16] = (byte) ((ret._etherType >> 8) & 0xff);
			ret._data[17] = (byte) (ret._etherType & 0xff);
			Array.Copy(ret._payload, 0, ret._data, 18, ret._payload.Length);

			return ret;
		}
	}
}
