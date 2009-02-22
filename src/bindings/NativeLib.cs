
using System;
using System.Runtime.InteropServices;

namespace TAP {
	public abstract class NativeLib {
		public abstract void set_callback(EthernetLogCallback cb);
		public abstract IntPtr init();
		public abstract void destroy(IntPtr tapcfg);
		public abstract int start(IntPtr tapcfg, string ifname, bool fallback);
		public abstract void stop(IntPtr tapcfg);
		public abstract int wait_readable(IntPtr tapcfg, int msec);
		public abstract int wait_writable(IntPtr tapcfg, int msec);
		public abstract int read(IntPtr tapcfg, byte[] buf, int count);
		public abstract int write(IntPtr tapcfg, byte[] buf, int count);
		public abstract string get_ifname(IntPtr tapcfg);
		public abstract IntPtr iface_get_hwaddr(IntPtr tapcfg, IntPtr length);
		public abstract int iface_get_status(IntPtr tapcfg);
		public abstract int iface_change_status(IntPtr tapcfg, int enabled);
		public abstract int iface_get_mtu(IntPtr tapcfg);
		public abstract int iface_set_mtu(IntPtr tapcfg, int mtu);
		public abstract int iface_set_ipv4(IntPtr tapcfg, string addr, byte netbits);

		public static NativeLib GetInstance() {
			if (IntPtr.Size == 8)
				return new NativeLib64();
			else
				return new NativeLib32();
		}

		private class NativeLib32 : NativeLib {
			public override void set_callback(EthernetLogCallback cb) {
				taplog_set_callback(cb);
			}

			public override IntPtr init() {
				return tapcfg_init();
			}

			public override void destroy(IntPtr tapcfg) {
				tapcfg_destroy(tapcfg);
			}

			public override int start(IntPtr tapcfg, string ifname, bool fallback) {
				return tapcfg_start(tapcfg, ifname, fallback ? 1 : 0);
			}

			public override void stop(IntPtr tapcfg) {
				tapcfg_stop(tapcfg);
			}

			public override int wait_readable(IntPtr tapcfg, int msec) {
				return tapcfg_wait_readable(tapcfg, msec);
			}

			public override int wait_writable(IntPtr tapcfg, int msec) {
				return tapcfg_wait_writable(tapcfg, msec);
			}

			public override int read(IntPtr tapcfg, byte[] buf, int count) {
				return tapcfg_read(tapcfg, buf, count);
			}

			public override int write(IntPtr tapcfg, byte[] buf, int count) {
				return tapcfg_write(tapcfg, buf, count);
			}

			public override string get_ifname(IntPtr tapcfg) {
				return tapcfg_get_ifname(tapcfg);
			}

			public override IntPtr iface_get_hwaddr(IntPtr tapcfg, IntPtr length) {
				return tapcfg_iface_get_hwaddr(tapcfg, length);
			}

			public override int iface_get_status(IntPtr tapcfg) {
				return tapcfg_iface_get_status(tapcfg);
			}

			public override int iface_change_status(IntPtr tapcfg, int enabled) {
				return tapcfg_iface_change_status(tapcfg, enabled);
			}

			public override int iface_get_mtu(IntPtr tapcfg) {
				return tapcfg_iface_get_mtu(tapcfg);
			}

			public override int iface_set_mtu(IntPtr tapcfg, int mtu) {
				return tapcfg_iface_set_mtu(tapcfg, mtu);
			}

			public override int iface_set_ipv4(IntPtr tapcfg, string addr, byte netbits) {
				return tapcfg_iface_set_ipv4(tapcfg, addr, netbits);
			}

			[DllImport("tapcfg")]
			private static extern void taplog_set_callback(EthernetLogCallback cb);

			[DllImport("tapcfg")]
			private static extern IntPtr tapcfg_init();
			[DllImport("tapcfg")]
			private static extern void tapcfg_destroy(IntPtr tapcfg);

			[DllImport("tapcfg")]
			private static extern int tapcfg_start(IntPtr tapcfg,
				[MarshalAs(UnmanagedType.CustomMarshaler,
					   MarshalTypeRef = typeof(UTF8Marshaler))]
				string ifname, int fallback);
			[DllImport("tapcfg")]
			private static extern void tapcfg_stop(IntPtr tapcfg);

			[DllImport("tapcfg")]
			private static extern int tapcfg_wait_readable(IntPtr tapcfg, int msec);
			[DllImport("tapcfg")]
			private static extern int tapcfg_wait_writable(IntPtr tapcfg, int msec);

			[DllImport("tapcfg")]
			private static extern int tapcfg_read(IntPtr tapcfg, byte[] buf, int count);
			[DllImport("tapcfg")]
			private static extern int tapcfg_write(IntPtr tapcfg, byte[] buf, int count);

			[DllImport("tapcfg")]
			[return : MarshalAs(UnmanagedType.CustomMarshaler,
					    MarshalTypeRef = typeof(UTF8Marshaler))]
			private static extern string tapcfg_get_ifname(IntPtr tapcfg);

			[DllImport("tapcfg")]
			private static extern IntPtr tapcfg_iface_get_hwaddr(IntPtr tapcfg, IntPtr length);

			[DllImport("tapcfg")]
			private static extern int tapcfg_iface_get_status(IntPtr tapcfg);
			[DllImport("tapcfg")]
			private static extern int tapcfg_iface_change_status(IntPtr tapcfg, int enabled);
			[DllImport("tapcfg")]
			private static extern int tapcfg_iface_get_mtu(IntPtr tapcfg);
			[DllImport("tapcfg")]
			private static extern int tapcfg_iface_set_mtu(IntPtr tapcfg, int mtu);
			[DllImport("tapcfg")]
			private static extern int tapcfg_iface_set_ipv4(IntPtr tapcfg, string addr, byte netbits);
		}

		private class NativeLib64 : NativeLib {
			public override void set_callback(EthernetLogCallback cb) {
				taplog_set_callback(cb);
			}

			public override IntPtr init() {
				return tapcfg_init();
			}

			public override void destroy(IntPtr tapcfg) {
				tapcfg_destroy(tapcfg);
			}

			public override int start(IntPtr tapcfg, string ifname, bool fallback) {
				return tapcfg_start(tapcfg, ifname, fallback ? 1 : 0);
			}

			public override void stop(IntPtr tapcfg) {
				tapcfg_stop(tapcfg);
			}

			public override int wait_readable(IntPtr tapcfg, int msec) {
				return tapcfg_wait_readable(tapcfg, msec);
			}

			public override int wait_writable(IntPtr tapcfg, int msec) {
				return tapcfg_wait_writable(tapcfg, msec);
			}

			public override int read(IntPtr tapcfg, byte[] buf, int count) {
				return tapcfg_read(tapcfg, buf, count);
			}

			public override int write(IntPtr tapcfg, byte[] buf, int count) {
				return tapcfg_write(tapcfg, buf, count);
			}

			public override string get_ifname(IntPtr tapcfg) {
				return tapcfg_get_ifname(tapcfg);
			}

			public override IntPtr iface_get_hwaddr(IntPtr tapcfg, IntPtr length) {
				return tapcfg_iface_get_hwaddr(tapcfg, length);
			}

			public override int iface_get_status(IntPtr tapcfg) {
				return tapcfg_iface_get_status(tapcfg);
			}

			public override int iface_change_status(IntPtr tapcfg, int enabled) {
				return tapcfg_iface_change_status(tapcfg, enabled);
			}

			public override int iface_get_mtu(IntPtr tapcfg) {
				return tapcfg_iface_get_mtu(tapcfg);
			}

			public override int iface_set_mtu(IntPtr tapcfg, int mtu) {
				return tapcfg_iface_set_mtu(tapcfg, mtu);
			}

			public override int iface_set_ipv4(IntPtr tapcfg, string addr, byte netbits) {
				return tapcfg_iface_set_ipv4(tapcfg, addr, netbits);
			}

			[DllImport("tapcfg64")]
			private static extern void taplog_set_callback(EthernetLogCallback cb);

			[DllImport("tapcfg64")]
			private static extern IntPtr tapcfg_init();
			[DllImport("tapcfg64")]
			private static extern void tapcfg_destroy(IntPtr tapcfg);

			[DllImport("tapcfg64")]
			private static extern int tapcfg_start(IntPtr tapcfg,
				[MarshalAs(UnmanagedType.CustomMarshaler,
					   MarshalTypeRef = typeof(UTF8Marshaler))]
				string ifname, int fallback);
			[DllImport("tapcfg64")]
			private static extern void tapcfg_stop(IntPtr tapcfg);

			[DllImport("tapcfg64")]
			private static extern int tapcfg_wait_readable(IntPtr tapcfg, int msec);
			[DllImport("tapcfg64")]
			private static extern int tapcfg_wait_writable(IntPtr tapcfg, int msec);

			[DllImport("tapcfg64")]
			private static extern int tapcfg_read(IntPtr tapcfg, byte[] buf, int count);
			[DllImport("tapcfg64")]
			private static extern int tapcfg_write(IntPtr tapcfg, byte[] buf, int count);

			[DllImport("tapcfg64")]
			[return : MarshalAs(UnmanagedType.CustomMarshaler,
					    MarshalTypeRef = typeof(UTF8Marshaler))]
			private static extern string tapcfg_get_ifname(IntPtr tapcfg);

			[DllImport("tapcfg64")]
			private static extern IntPtr tapcfg_iface_get_hwaddr(IntPtr tapcfg, IntPtr length);

			[DllImport("tapcfg64")]
			private static extern int tapcfg_iface_get_status(IntPtr tapcfg);
			[DllImport("tapcfg64")]
			private static extern int tapcfg_iface_change_status(IntPtr tapcfg, int enabled);
			[DllImport("tapcfg64")]
			private static extern int tapcfg_iface_get_mtu(IntPtr tapcfg);
			[DllImport("tapcfg64")]
			private static extern int tapcfg_iface_set_mtu(IntPtr tapcfg, int mtu);
			[DllImport("tapcfg64")]
			private static extern int tapcfg_iface_set_ipv4(IntPtr tapcfg, string addr, byte netbits);
		}
	}
}

