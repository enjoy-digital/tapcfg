
using System;
using System.Runtime.InteropServices;

namespace TAPCfg {
	public class EthernetDevice : IDisposable {
		private const int MTU = 1522;

		private IntPtr handle;
		private bool disposed = false;

		public EthernetDevice() {
			handle = tapcfg_init();
		}

		public void Start() {
			tapcfg_start(handle);
		}

		public byte[] Read() {
			int length;
			byte[] buffer = new byte[MTU];

			length = tapcfg_read(handle, buffer, buffer.Length);

			byte[] outbuf = new byte[length];
			Array.Copy(buffer, 0, outbuf, 0, length);

			return outbuf;
		}

		public void Write(byte[] data) {
			byte[] buffer = new byte[MTU];

			Array.Copy(data, 0, buffer, 0, data.Length);
			int ret = tapcfg_write(handle, buffer, data.Length);
		}

		public void Enabled(bool enabled) {
			if (enabled)
				tapcfg_iface_change_status(handle, 1);
			else
				tapcfg_iface_change_status(handle, 0);
		}

		public void Dispose() {
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing) {
			if (!disposed) {
				if (disposing) {
					// Managed resources can be disposed here
				}

				tapcfg_stop(handle);
				tapcfg_destroy(handle);
				handle = IntPtr.Zero;

				disposed = true;
			}
		}

		private static void Main(string[] args) {
			EthernetDevice dev = new EthernetDevice();
			dev.Start();
			dev.Enabled(true);

			System.Threading.Thread.Sleep(100000);
		}

		[DllImport("libtapcfg")]
		private static extern IntPtr tapcfg_init();
		[DllImport("libtapcfg")]
		private static extern void tapcfg_destroy(IntPtr tapcfg);

		[DllImport("libtapcfg")]
		private static extern int tapcfg_start(IntPtr tapcfg);
		[DllImport("libtapcfg")]
		private static extern void tapcfg_stop(IntPtr tapcfg);

		[DllImport("libtapcfg")]
		private static extern int tapcfg_has_data(IntPtr tapcfg);
		[DllImport("libtapcfg")]
		private static extern int tapcfg_read(IntPtr tapcfg, byte[] buf, int count);
		[DllImport("libtapcfg")]
		private static extern int tapcfg_write(IntPtr tapcfg, byte[] buf, int count);

		[DllImport("libtapcfg")]
		private static extern string tapcfg_get_ifname(IntPtr tapcfg);

		[DllImport("libtapcfg")]
		private static extern int tapcfg_iface_get_status(IntPtr tapcfg);
		[DllImport("libtapcfg")]
		private static extern int tapcfg_iface_change_status(IntPtr tapcfg, int enabled);
		[DllImport("libtapcfg")]
		private static extern int tapcfg_iface_set_ipv4(IntPtr tapcfg, string addr, Byte netbits);
		[DllImport("libtapcfg")]
		private static extern int tapcfg_iface_set_ipv6(IntPtr tapcfg, string addr, Byte netbits);
	}
}
