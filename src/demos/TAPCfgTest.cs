
using TAP;
using System;
using System.Net;

public class TAPCfgTest {
	private static void Main(string[] args) {
		EthernetDevice dev = new EthernetDevice();
		dev.Start("Device name");
		Console.WriteLine("Got device name: {0}", dev.DeviceName);
		dev.MTU = 1280;
		dev.SetAddress(IPAddress.Parse("192.168.10.1"), 16);
//		dev.SetAddress(IPAddress.Parse("fc00::1"), 64);
		dev.Enabled = true;

		while (true) {
			EthernetFrame frame = dev.Read();
			if (frame == null)
				break;

			Console.WriteLine("Read Ethernet frame of type {0}",
			                  frame.EtherType);

			if (frame.EtherType == EtherType.IPv6) {
				IPv6Packet packet = new IPv6Packet(frame.Payload);
				Console.WriteLine("Source \"{0}\" Destination \"{1}\"",
				                  packet.Source, packet.Destination);
				if (packet.NextHeader == ProtocolType.ICMPv6) {
					ICMPv6Type type = (ICMPv6Type) packet.Payload[0];
					Console.WriteLine("ICMPv6 packet type {0}", type);
					Console.WriteLine("Data: {0}", BitConverter.ToString(packet.Payload));

					if (type == ICMPv6Type.RouterSolicitation) {
						sendRouterAdv(dev);
					}
				}
			} else if (frame.EtherType == EtherType.IPv4) {
				IPv4Packet packet = new IPv4Packet(frame.Payload);
				Console.WriteLine("Source {0} Destination {1}",
				                  packet.Source, packet.Destination);
			}

			Console.WriteLine("Source address: {0}",
			                  BitConverter.ToString(frame.Source));
			Console.WriteLine("Destination address: {0}",
			                  BitConverter.ToString(frame.Destination));
		}
	}

	private static void sendRouterAdv(EthernetDevice dev) {
		NDRouterAdvPacket adv =
			new NDRouterAdvPacket(IPAddress.Parse("fc00::"));
		adv.Source = IPAddress.Parse("fe80::211:24ff:fe93:3b66");
		adv.Destination = IPAddress.Parse("ff02::1");
		adv.HopLimit = 255;
		byte[] adv_data = adv.Data;

		byte[] frame_data = new byte[14 + adv_data.Length];
		Array.Copy(new byte[] { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
			   0, frame_data, 0, 6);
		Array.Copy(new byte[] { 0x01, 0x11, 0x24, 0x93, 0x3b, 0x66 },
			   0, frame_data, 6, 6);
		frame_data[12] = (byte) ((int) EtherType.IPv6 >> 8);
		frame_data[13] = (byte) ((int) EtherType.IPv6 & 0xff);
		Array.Copy(adv_data, 0, frame_data, 14, adv_data.Length);

		EthernetFrame fr = new EthernetFrame(frame_data);
		Console.WriteLine("Wrote packet {0}", BitConverter.ToString(fr.Data));
		dev.Write(fr);
	}
}
