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
using System.Net;
using System.Net.Sockets;

namespace TAP {
	/* http://www.iana.org/assignments/icmpv6-parameters */
	public enum ICMPv6Type : byte {
		/* RFC4443 */
		DestinationUnreachable        = 1,
		PacketTooBig                  = 2,
		TimeExceeded                  = 3,
		ParameterProblem              = 4,
		EchoRequest                   = 128,
		EchoReply                     = 129,

		/* RFC2710 */
		MulticastListenerQuery        = 130,
		MulticastListenerReport       = 131,
		MulticastListenerDone         = 132,

		/* RFC4861 */
		RouterSolicitation            = 133,
		RouterAdvertisement           = 134,
		NeighborSolicitation          = 135,
		NeighborAdvertisement         = 136,
		Redirect                      = 137,

		/* Crawford */
		RouterRenumbering             = 138,

		/* RFC4620 */
		NodeInformationQuery          = 139,
		NodeInformationResponse       = 140,

		/* RFC3122 */
		INDiscoverySolicitation       = 141,
		INDiscoveryAdvertisement      = 142,

		/* RFC3810 */
		V2MulticastListenerReport     = 143,

		/* RFC3775 */
		HAADiscoveryRequest           = 144,
		HAADiscoveryReply             = 145,
		MobilePrefixSolicitation      = 146,
		MobilePrefixAdvertisement     = 147,

		/* RFC3971 */
		CertPathSolicitation          = 148,
		CertPathAdvertisement         = 149,

		/* RFC4286 */
		MulticastRouterAdvertisement  = 151,
		MulticastRouterSolicitation   = 152,
		MulticastRouterTermination    = 153,

		/* RFC5268 */
		FMIPv6                        = 154
	};

	public class ICMPv6Packet : IPv6Packet {
		protected byte _icmp_type;
		protected byte _icmp_code;
		protected byte[] _icmp_payload;

		public override byte[] Data {
			get {
				_next_header = (byte) ProtocolType.ICMPv6;
				_ip_payload = new byte[4 + _icmp_payload.Length];
				_ip_payload[0] = _icmp_type;
				_ip_payload[1] = _icmp_code;
				_ip_payload[2] = 0;
				_ip_payload[3] = 0;
				Array.Copy(_icmp_payload, 0, _ip_payload, 4,
				           _icmp_payload.Length);

				/* Save the checksum to the payload */
				int checksum = calculateChecksum();
				_ip_payload[2] = (byte) ((checksum >> 8) & 0xff);
				_ip_payload[3] = (byte) (checksum & 0xff);

				return base.Data;
			}
		}

		public ICMPv6Type Type {
			get { return (ICMPv6Type) _icmp_type; }
			set { _icmp_type = (byte) value; }
		}

		public byte Code {
			get { return _icmp_code; }
			set { _icmp_code = value; }
		}

		public override byte[] Payload {
			get { return _icmp_payload; }
			set { _icmp_payload = value; }
		}
	}

	public class IPv6Packet {
		protected byte _traffic_class;
		protected int _flow_label;
		protected byte _next_header;
		protected byte _hop_limit;

		protected byte[] _src = new byte[16];
		protected byte[] _dst = new byte[16];

		protected byte[] _ip_payload = new byte[0];

		public IPv6Packet() {
			_traffic_class = 0;
			_flow_label = 0;
			_next_header = 0;
			_hop_limit = 0;
		}

		public static IPv6Packet Parse(byte[] data) {
			if (data.Length < 40) {
				throw new Exception("IPv6 packet too small to include a header");
			}

			IPv6Packet packet = new IPv6Packet();

			int version = (data[0] >> 4) & 0x0f;
			packet._traffic_class = (byte) (((data[0] & 0x0f) << 4) |
			                                ((data[1] & 0xf0) >> 4));
			packet._flow_label = ((data[1] & 0x0f) << 16) |
			                      (data[2] << 8) | data[3];
			int payload_length = (data[4] << 8) | data[5];
			packet._next_header = data[6];
			packet._hop_limit = data[7];
			Array.Copy(data,  8, packet._src, 0, 16);
			Array.Copy(data, 24, packet._dst, 0, 16);

			if (version != 6) {
				throw new Exception("IPv6 packet version field not 6");
			}

			if (payload_length > data.Length - 40) {
				throw new Exception("Payload length longer than actual data");
			}

			packet._ip_payload = new byte[payload_length];
			Array.Copy(data, 40, packet._ip_payload, 0, payload_length);

			return packet;
		}

		public virtual byte[] Data {
			get {
				byte[] data = new byte[40 + _ip_payload.Length];
				data[0] = (byte) ((6 << 4) | ((_traffic_class >> 4) & 0x0f));
				data[1] = (byte) (((_traffic_class << 4) & 0xf0) |
				                  ((_flow_label >> 16) & 0x0f));
				data[2] = (byte) ((_flow_label >> 8) & 0xff);
				data[3] = (byte) (_flow_label & 0xff);
				data[4] = (byte) ((_ip_payload.Length >> 8) & 0xff);
				data[5] = (byte) (_ip_payload.Length & 0xff);
				data[6] = _next_header;
				data[7] = _hop_limit;
				Array.Copy(_src, 0, data, 8, 16);
				Array.Copy(_dst, 0, data, 24, 16);
				Array.Copy(_ip_payload, 0, data, 40, _ip_payload.Length);
				return data;
			}
		}

		public byte TrafficClass {
			get { return _traffic_class; }
			set { _traffic_class = value; }
		}

		public int FlowLabel {
			get { return _flow_label; }
			set {
				if (value > 0x0fffff)
					throw new Exception("Flow label value too big");
				_flow_label = value;
			}
		}

		public ProtocolType NextHeader {
			get { return (ProtocolType) _next_header; }
			set { _next_header = (byte) value; }
		}

		public byte HopLimit {
			get { return _hop_limit; }
			set { _hop_limit = value; }
		}

		public IPAddress Source {
			get { return new IPAddress(_src); }
			set {
				if (value.AddressFamily != AddressFamily.InterNetworkV6)
					throw new Exception("Source address not an IPv6 address");
				_src = value.GetAddressBytes();
			}
		}

		public IPAddress Destination {
			get { return new IPAddress(_dst); }
			set {
				if (value.AddressFamily != AddressFamily.InterNetworkV6)
					throw new Exception("Source address not an IPv6 address");
				_dst = value.GetAddressBytes();
			}
		}

		public virtual byte[] Payload {
			get { return _ip_payload; }
			set { _ip_payload = value; }
		}

		protected int calculateChecksum() {
			int checksum = 0;

			for (int i=0; i<_ip_payload.Length; i++) {
				checksum += _ip_payload[i] << ((i%2 == 0) ? 8 : 0);
				if (checksum > 0xffff) {
					checksum = (checksum & 0xffff) + 1;
				}
			}

			for (int i=0; i<16; i+=2) {
				checksum += (_src[i] << 8) | _src[i+1];
				checksum += (_dst[i] << 8) | _dst[i+1];
			}
			checksum += _ip_payload.Length;
			checksum += _next_header;
			if (checksum > 0xffff) {
				checksum = (checksum & 0xffff) + (checksum >> 16);
			}

			return ~checksum;
		}
	}
}
