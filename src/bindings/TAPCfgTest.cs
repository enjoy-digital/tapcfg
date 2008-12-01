
using TAP;
using System;
using System.Net;

public class TAPCfgTest {
	private static void Main(string[] args) {
		EthernetDevice dev = new EthernetDevice();
		dev.Start("Device name");
		Console.WriteLine("Got device name: {0}", dev.DeviceName);
		dev.MTU = 1280;
		dev.SetAddress(IPAddress.Parse("192.168.1.1"), 16);
		dev.SetAddress(IPAddress.Parse("fc00::1"), 64);
		dev.Enabled = true;

		while (true) {
			EthernetFrame frame = dev.Read();
			if (frame == null)
				break;

			Console.WriteLine("Read Ethernet frame of type {0}",
			                  frame.EtherType);
			Console.WriteLine("Source address: {0}",
			                  BitConverter.ToString(frame.SourceAddress));
			Console.WriteLine("Destination address: {0}",
			                  BitConverter.ToString(frame.DestinationAddress));
		}
	}
}
