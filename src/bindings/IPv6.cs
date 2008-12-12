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

	public class IPv6Packet {
		private byte _traffic_class;
		private int _flow_label;
		private byte _next_header;
		private byte _hop_limit;

		private byte[] _src = new byte[16];
		private byte[] _dst = new byte[16];

		private byte[] _payload = new byte[0];;

		public IPv6Packet() {
		}

		public IPv6Packet(byte[] data) {
			if (data.Length < 40) {
				throw new Exception("IPv6 packet too small to include a header");
			}

			int version = (data[0] >> 4) & 0x0f;
			_traffic_class = (byte) (((data[0] & 0x0f) << 4) | ((data[1] & 0xf0) >> 4));
			_flow_label = ((data[1] & 0x0f) << 16) | (data[2] << 8) | data[3];
			int payload_length = (data[4] << 8) | data[5];
			_next_header = data[6];
			_hop_limit = data[7];
			Array.Copy(data,  8, _src, 0, 16);
			Array.Copy(data, 24, _dst, 0, 16);

			if (version != 6) {
				throw new Exception("IPv6 packet version field not 6");
			}

			if (payload_length > data.Length - 40) {
				throw new Exception("Payload length longer than actual data");
			}

			_payload = new byte[payload_length];
			Array.Copy(data, 40, _payload, 0, payload_length);
		}

		public byte[] Data {
			get {
				byte[] data = new byte[40 + _payload.Length];
				data[0] = (byte) ((6 << 4) | ((_traffic_class >> 4) & 0x0f));
				data[1] = (byte) (((_traffic_class << 4) & 0xf0) |
				                  ((_flow_label >> 16) & 0x0f));
				data[2] = (byte) ((_flow_label >> 8) & 0xff);
				data[3] = (byte) (_flow_label & 0xff);
				data[4] = (byte) ((_payload.Length >> 8) & 0xff);
				data[5] = (byte) (_payload.Length & 0xff);
				data[6] = _next_header;
				data[7] = _hop_limit;
				Array.Copy(_src, 0, data, 8, 16);
				Array.Copy(_dst, 0, data, 24, 16);
				Array.Copy(_payload, 0, data, 40, _payload.Length);
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

		public byte[] Payload {
			get { return _payload; }
			set { _payload = value; }
		}
	}
}
