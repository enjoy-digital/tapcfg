
using TAP;
using System;
using System.Net;
using System.Threading;

public class TAPCfgTest {
	private static void Main(string[] args) {
		EthernetDevice dev = new EthernetDevice();
		dev.Start("Device name");
		Console.WriteLine("Got device name: {0}", dev.DeviceName);
		dev.MTU = 1280;
//		dev.SetAddress(IPAddress.Parse("192.168.10.1"), 16);
		dev.Enabled = true;

		PacketMangler mangler = new PacketMangler(dev);
		mangler.Start();

		while (true) {
			Thread.Sleep(1000);
		}
	}
}
