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
using System.Net;
using System.Net.Sockets;
using System.Collections;

namespace TAP {
	public class NDPrefixInfo {
		private byte _prefix_len;
		private byte _flags_reserved = 0;

		private int _valid_time;
		private int _preferred_time;

		private byte[] _prefix;

		public NDPrefixInfo(IPAddress prefix, byte length) {
			this.AdvOnLink     = true;
			this.AdvAutonomous = true;
			this.AdvRouterAddr = false;
			_valid_time        = 2592000;
			_preferred_time    = 604800;

			this.Prefix = prefix;
			this.PrefixLength = length;
		}

		public byte[] Data {
			get {
				byte[] ret = new byte[32];
				ret[0]  = 3;               /* ND_OPT_PREFIX_INFORMATION  */
				ret[1]  = 4;               /* len */
				ret[2]  = _prefix_len;
				ret[3]  = _flags_reserved;
				ret[4]  = (byte) ((_valid_time >> 24) & 0xff);
				ret[5]  = (byte) ((_valid_time >> 16) & 0xff);
				ret[6]  = (byte) ((_valid_time >>  8) & 0xff);
				ret[7]  = (byte) (_valid_time & 0xff);
				ret[8]  = (byte) ((_preferred_time >> 24) & 0xff);
				ret[9]  = (byte) ((_preferred_time >> 16) & 0xff);
				ret[10] = (byte) ((_preferred_time >>  8) & 0xff);
				ret[11] = (byte) (_preferred_time & 0xff);
				ret[12] = ret[13] = ret[14] = ret[15] = 0;
				Array.Copy(_prefix, 0, ret, 16, 16);
				return ret;
			}
		}

		public static int Length {
			get { return 32; }
		}

		public bool AdvOnLink {
			get { return (_flags_reserved & 0x80) != 0; }
			set { setFlag(0x80, value); }
		}

		public bool AdvAutonomous {
			get { return (_flags_reserved & 0x40) != 0; }
			set { setFlag(0x40, value); }
		}

		public bool AdvRouterAddr {
			get { return (_flags_reserved & 0x20) != 0; }
			set { setFlag(0x20, value); }
		}

		public int AdvValidLifetime {
			get { return _valid_time; }
			set { _valid_time = value; }
		}

		public int AdvPreferredLifetime {
			get { return _preferred_time; }
			set { _preferred_time = value; }
		}

		public IPAddress Prefix {
			get { return new IPAddress(_prefix); }
			set {
				if (value.AddressFamily != AddressFamily.InterNetworkV6)
					throw new Exception("Prefix address not an IPv6 address");
				_prefix = value.GetAddressBytes();
			}
		}

		public byte PrefixLength {
			get { return _prefix_len; }
			set { _prefix_len = value; }
		}

		private void setFlag(byte mask, bool value) {
			if (value) {
				_flags_reserved |= mask;
			} else {
				_flags_reserved &= (byte) (~mask);
			}
		}
	}

	public class NDRouterAdvPacket : ICMPv6Packet {
		protected byte _curhoplimit;
		protected byte _flags_reserved;
		protected short _router_lifetime;
		protected int _reachable;
		protected int _retransmit;

		private ArrayList _prefixes = new ArrayList();

		public NDRouterAdvPacket() {
			_curhoplimit = 64;
			this.AdvManagedFlag = false;
			this.AdvOtherConfigFlag = false;
			this.AdvHomeAgentFlag = false;
			_router_lifetime = 0;
			_reachable = 0;
			_retransmit = 0;
		}

		public override byte[] Data {
			get {
				_icmp_type = (byte) ICMPv6Type.RouterAdvertisement;

				int prefixes_length = _prefixes.Count * NDPrefixInfo.Length;;
				_icmp_payload = new byte[12 + prefixes_length];
				_icmp_payload[0]  = _curhoplimit;
				_icmp_payload[1]  = _flags_reserved;
				_icmp_payload[2]  = (byte) ((_router_lifetime >> 8) & 0xff);
				_icmp_payload[3]  = (byte) (_router_lifetime & 0xff);
				_icmp_payload[4]  = (byte) ((_reachable >> 24) & 0xff);
				_icmp_payload[5]  = (byte) ((_reachable >> 16) & 0xff);
				_icmp_payload[6] = (byte) ((_reachable >>  8) & 0xff);
				_icmp_payload[7] = (byte) (_reachable & 0xff);
				_icmp_payload[8] = (byte) ((_retransmit >> 24) & 0xff);
				_icmp_payload[9] = (byte) ((_retransmit >> 16) & 0xff);
				_icmp_payload[10] = (byte) ((_retransmit >>  8) & 0xff);
				_icmp_payload[11] = (byte) (_retransmit & 0xff);

				int offset = 12;
				foreach (NDPrefixInfo prefix in _prefixes) {
					byte[] prefix_data = prefix.Data;
					Array.Copy(prefix_data, 0,
					           _icmp_payload, offset,
					           prefix_data.Length);
					offset += prefix_data.Length;
				}

				return base.Data;
			}
		}

		public byte AdvCurHopLimit {
			get { return _curhoplimit; }
			set { _curhoplimit = value; }
		}

		public bool AdvManagedFlag {
			get { return (_flags_reserved & 0x80) != 0; }
			set { setFlag(0x80, value); }
		}

		public bool AdvOtherConfigFlag {
			get { return (_flags_reserved & 0x40) != 0; }
			set { setFlag(0x40, value); }
		}

		public bool AdvHomeAgentFlag {
			get { return (_flags_reserved & 0x20) != 0; }
			set { setFlag(0x20, value); }
		}

		public short AdvDefaultLifetime {
			get { return _router_lifetime; }
			set { _router_lifetime = value; }
		}

		public int AdvReachableTime {
			get { return _reachable; }
			set { _reachable = value; }
		}

		public int AdvRetransTime {
			get { return _retransmit; }
			set { _retransmit = value; }
		}

		public void AddPrefix(IPAddress address, byte prefixlen) {
			NDPrefixInfo prefix = new NDPrefixInfo(address, prefixlen);
			_prefixes.Add(prefix);
		}

		private void setFlag(byte mask, bool value) {
			if (value) {
				_flags_reserved |= mask;
			} else {
				_flags_reserved &= (byte) (~mask);
			}
		}
	}
}
