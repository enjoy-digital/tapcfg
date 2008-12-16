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
	using System;
	using System.Net;
	using System.Timers;
	using System.Security.Cryptography;

	public class VirtualRouter : INetworkHost {
		private Timer _timer;

		private byte[] _mac;
		private byte[] _prefix;
		private IPAddress _linklocal;
		private PacketMangler _mangler;

		public VirtualRouter(PacketMangler mangler) {
			/* Create MAC and ULA addresses */
			_mac = createMAC();
			_prefix = createPrefix(_mac);

			/* Choose a random subnet from the prefix */
			int subnet = (new Random()).Next(0, 0xffff);
			_prefix[6] = (byte) ((subnet >> 8) & 0xff);
			_prefix[7] = (byte) (subnet & 0xff);

			/* Create IPv6 link local address from MAC */
			byte[] data = new byte[16];
			data[0] = 0xfe;
			data[1] = 0x80;
			Array.Copy(getEUI64(_mac), 0, data, 8, 8);
			_linklocal = new IPAddress(data);

			_mangler = mangler;
		}

		public bool IsRouter() {
			return true;
		}

		public void HandleFrame(EthernetFrame frame) {
		}

		public void Advertise() {
			_timer = new Timer();
			_timer.Elapsed += new ElapsedEventHandler(timerEvent);
			_timer.Interval = 30000;
			_timer.Start();
		}

		private void timerEvent(object source, ElapsedEventArgs e) {
			SendRouterAdv(null);
		}

		public void SendRouterAdv(IPAddress dest) {
			NDRouterAdvPacket adv = new NDRouterAdvPacket(new IPAddress(_prefix));
			adv.Source = _linklocal;
			if (dest != null) {
				adv.Destination = dest;
			} else {
				adv.Destination = IPAddress.Parse("ff02::1");
			}
			/* This has to be 255 to make sure it is not forwarded */
			adv.HopLimit = 255;
			byte[] adv_data = adv.Data;

			byte[] frame_data = new byte[14 + adv_data.Length];
			Array.Copy(EthernetFrame.Broadcast, 0, frame_data, 0, 6);
			Array.Copy(_mac, 0, frame_data, 6, 6);
			frame_data[12] = (byte) ((int) EtherType.IPv6 >> 8);
			frame_data[13] = (byte) ((int) EtherType.IPv6 & 0xff);
			Array.Copy(adv_data, 0, frame_data, 14, adv_data.Length);

			_mangler.MangleFrame(this, new EthernetFrame(frame_data));
		}

		private byte[] createMAC() {
			byte[] ret = new byte[6];
			Random random = new Random();

			for (int i=0; i<6; i++) {
				ret[i] = (byte) random.Next(0, 255);
			}
			/* It's better to have the universal/local bit 0 */
			ret[0] = (byte) (ret[0] & ~0x02);

			return ret;
		}

		private byte[] createPrefix(byte[] mac) {
			byte[] time = getNTPTime(DateTime.Now);
			byte[] eui64 = getEUI64(mac);

			byte[] data = new byte[16];
			Array.Copy(time, 0, data, 0, 8);
			Array.Copy(eui64, 0, data, 8, 8);

			SHA1 sha = new SHA1CryptoServiceProvider(); 
			byte[] hash = sha.ComputeHash(data);

			byte[] address = new byte[16];
			address[0] = 0xfc | 0x01;
			Array.Copy(hash, 15, address, 1, 5);

			return address;
		}

		private byte[] getEUI64(byte[] mac) {
			byte[] ret = new byte[8];

			ret[0] = (byte) (mac[0] | 0x02);
			ret[1] = mac[1];
			ret[2] = mac[2];
			ret[3] = 0xff;
			ret[4] = 0xfe;
			ret[5] = mac[3];
			ret[6] = mac[4];
			ret[7] = mac[5];

			return ret;
		}

		private byte[] getNTPTime(DateTime date) {
			ulong millisec = (ulong) (date - new DateTime(1900, 1, 1, 0, 0, 0)).TotalMilliseconds;

			ulong intpart = millisec / 1000;
			ulong fracpart = ((millisec % 1000) * 0x100000000L) / 1000;

			byte[] ret = new byte[8];
			for (int i=3; i>=0; i--) {
				ret[i] = (byte) (intpart & 0xff);
				intpart >>= 8;
			}
			for (int i=7; i>=4; i--) {
				ret[i] = (byte) (fracpart & 0xff);
				fracpart >>= 8;
			}

			return ret;
		}
	}
}
