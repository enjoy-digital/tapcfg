
using TAPNet;
using System;
using System.Net;
using System.Threading;

public class TAPNetTest {
	private static void Main(string[] args) {
		EthernetDevice dev = new EthernetDevice();
		dev.Start("Device name", true);
		Console.WriteLine("Got device name: {0}", dev.DeviceName);
		Console.WriteLine("Got device hwaddr: {0}", BitConverter.ToString(dev.HWAddress));
		dev.HWAddress = new byte[] { 0x00, 0x01, 0x23, 0x45, 0x67, 0x89 };
		dev.MTU = 1280;
		dev.SetAddress(IPAddress.Parse("192.168.10.1"), 16);
		dev.Enabled = true;

		while (true) {
			Thread.Sleep(1000);
		}
	}
}
