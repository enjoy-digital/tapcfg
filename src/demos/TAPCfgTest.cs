
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
		dev.Enabled = true;

		VirtualRouter router = new VirtualRouter();

		while (true) {
			EthernetFrame frame = dev.Read();
			if (frame == null)
				break;

			Console.WriteLine("Read Ethernet frame of type {0}",
			                  frame.EtherType);

			if (frame.EtherType == EtherType.IPv6) {
				IPv6Packet packet = IPv6Packet.Parse(frame.Payload);
				Console.WriteLine("Source \"{0}\" Destination \"{1}\"",
				                  packet.Source, packet.Destination);
				if (packet.NextHeader == ProtocolType.ICMPv6) {
					ICMPv6Type type = (ICMPv6Type) packet.Payload[0];
					Console.WriteLine("ICMPv6 packet type {0}", type);
					Console.WriteLine("Data: {0}", BitConverter.ToString(packet.Payload));

					if (type == ICMPv6Type.RouterSolicitation) {
						sendRouterAdv(dev, router, packet.Source);
					}
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

	private static void sendRouterAdv(EthernetDevice dev, VirtualRouter router, IPAddress dest) {
		EthernetFrame fr = router.CreateRouterAdv(dest);
		Console.WriteLine("Wrote packet {0}", BitConverter.ToString(fr.Data));
		dev.Write(fr);
	}
}
