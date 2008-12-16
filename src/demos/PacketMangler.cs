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

using TAP;
using System;
using System.Collections;

namespace TAP {
	public class PacketMangler {
		ArrayList _remotehosts = new ArrayList();
		Hashtable _rarptable = new Hashtable();
		Hashtable _arptable = new Hashtable();

		LocalHost _localhost;
		VirtualRouter _router;

		public PacketMangler(EthernetDevice dev) {
			_localhost = new LocalHost(dev, this);
			_router = new VirtualRouter();
		}

		public void Start() {
			_localhost.Start();
		}

		public void AddRemoteHost(INetworkHost host) {
			_remotehosts.Add(host);
		}

		public void MangleFrame(INetworkHost source, EthernetFrame frame) {

			printInfo(frame);

			/* If the host is not in our RARP table, add it */
			if (_rarptable[frame.Source] == null) {
				_rarptable.Add(frame.Source, source);
			}

			if (frame.EtherType == EtherType.IPv4) {
				/* IPv4 specific packet handling */
				byte[] payload = frame.Payload;
				byte[] ipv4_source = new byte[4];
				Array.Copy(payload, 12, ipv4_source, 0, 4);

				/* Add host to the ARP table if not there */
				if (_arptable[ipv4_source] == null) {
					_arptable.Add(ipv4_source, frame.Source);
				}
			} else if (frame.EtherType == EtherType.IPv6) {
				/* IPv6 specific packet handling */
				IPv6Packet packet = IPv6Packet.Parse(frame.Payload);
				if (packet.NextHeader == ProtocolType.ICMPv6) {
					ICMPv6Type type = (ICMPv6Type) packet.Payload[0];
					if (type == ICMPv6Type.RouterSolicitation) {
						EthernetFrame fr = _router.CreateRouterAdv(packet.Source);
						Console.WriteLine("Wrote packet {0}", BitConverter.ToString(fr.Data));
						source.HandleFrame(fr);
					}
				}
				
			}

			INetworkHost dest = (INetworkHost) _rarptable[frame.Destination];
			if (dest != null) {
				dest.HandleFrame(frame);
			} else {
				foreach (INetworkHost host in _remotehosts) {
					host.HandleFrame(frame);
				}
			}
		}

		private void printInfo(EthernetFrame frame) {
			Console.WriteLine("Read Ethernet frame of type {0}",
			                  frame.EtherType);

			if (frame.EtherType == EtherType.IPv6) {
				IPv6Packet packet = IPv6Packet.Parse(frame.Payload);
				Console.WriteLine("Source \"{0}\" Destination \"{1}\"",
				                  packet.Source, packet.Destination);
				Console.WriteLine("Next header: {0}", packet.NextHeader);
				if (packet.NextHeader == ProtocolType.ICMPv6) {
					ICMPv6Type type = (ICMPv6Type) packet.Payload[0];
					Console.WriteLine("ICMPv6 packet type {0}", type);
					Console.WriteLine("Data: {0}", BitConverter.ToString(packet.Payload));
				}
			} else if (frame.EtherType == EtherType.IPv4) {
				IPv4Packet packet = IPv4Packet.Parse(frame.Payload);
				Console.WriteLine("Source {0} Destination {1}",
				                  packet.Source, packet.Destination);
			}

			Console.WriteLine("Source address: {0}",
			                  BitConverter.ToString(frame.Source));
			Console.WriteLine("Destination address: {0}",
			                  BitConverter.ToString(frame.Destination));
		}
	}
}
