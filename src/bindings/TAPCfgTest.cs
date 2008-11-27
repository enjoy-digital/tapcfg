
using TAP;
using System.Net;

public class TAPCfgTest {
	private static void Main(string[] args) {
		EthernetDevice dev = new EthernetDevice();
		dev.Start();
		dev.SetAddress(IPAddress.Parse("192.168.1.1"), 16);
		dev.SetAddress(IPAddress.Parse("fc00::1"), 64);
		dev.Enabled = true;

		System.Threading.Thread.Sleep(100000);
	}
}
