/**
 *  tapcfg - A cross-platform configuration utility for TAP driver
 *  Copyright (C) 2008-2010  Juho Vähä-Herttua
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
using System.Runtime.InteropServices;

namespace TAPNet {
	public enum LogLevel {
		Unknown,
		Emergency,
		Alert,
		Critical,
		Error,
		Warning,
		Notice,
		Info,
		Debug
	};
	public delegate void LogCallback(LogLevel level, string msg);

	public abstract class NativeLib {
		public abstract void set_log_callback(IntPtr tapcfg, LogCallback cb);
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
		public abstract int iface_set_hwaddr(IntPtr tapcfg, byte[] hwaddr, int length);
		public abstract int iface_get_status(IntPtr tapcfg);
		public abstract int iface_set_status(IntPtr tapcfg, int flags);
		public abstract int iface_get_mtu(IntPtr tapcfg);
		public abstract int iface_set_mtu(IntPtr tapcfg, int mtu);
		public abstract int iface_set_ipv4(IntPtr tapcfg, string addr, byte netbits);
		public abstract int iface_set_dhcp_options(IntPtr tapcfg, byte[] buffer, int buflen);

		public static NativeLib GetInstance() {
			if (IntPtr.Size == 8)
				return new NativeLib64();
			else
				return new NativeLib32();
		}

		private delegate void InternalLogCallback(int level, IntPtr msg_ptr);

		private LogCallback _logCallback = null;
		private void MarshalLogCallback(int level, IntPtr msg_ptr) {
			LogLevel logLevel = LogLevel.Unknown;
			if (level == 0)
				logLevel = LogLevel.Emergency;
			else if (level == 1)
				logLevel = LogLevel.Alert;
			else if (level == 2)
				logLevel = LogLevel.Critical;
			else if (level == 3)
				logLevel = LogLevel.Error;
			else if (level == 4)
				logLevel = LogLevel.Warning;
			else if (level == 5)
				logLevel = LogLevel.Notice;
			else if (level == 6)
				logLevel = LogLevel.Info;
			else if (level == 7)
				logLevel = LogLevel.Debug;

			ICustomMarshaler marshaler = new UTF8Marshaler();
			string msg = (string) marshaler.MarshalNativeToManaged(msg_ptr);
			_logCallback(logLevel, msg);
		}

		private class NativeLib32 : NativeLib {
			public override void set_log_callback(IntPtr tapcfg, LogCallback cb) {
				_logCallback = cb;
				if (_logCallback != null) {
					tapcfg_set_log_callback(tapcfg, new InternalLogCallback(MarshalLogCallback));
				} else {
					tapcfg_set_log_callback(tapcfg, null);
				}
			}

			public override IntPtr init() {
				return tapcfg_init();
			}

			public override void destroy(IntPtr tapcfg) {
				tapcfg_destroy(tapcfg);
			}

			public override int start(IntPtr tapcfg, string ifname, bool fallback) {
				ICustomMarshaler marshaler = new UTF8Marshaler();
				IntPtr ifname_ptr = marshaler.MarshalManagedToNative(ifname);
				int ret = tapcfg_start(tapcfg, ifname_ptr, fallback ? 1 : 0);
				marshaler.CleanUpNativeData(ifname_ptr);
				return ret;
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
				ICustomMarshaler marshaler = new UTF8Marshaler();
				IntPtr ret_ptr = tapcfg_get_ifname(tapcfg);
				string ret = (string) marshaler.MarshalNativeToManaged(ret_ptr);
				marshaler.CleanUpNativeData(ret_ptr);
				return ret;
			}

			public override IntPtr iface_get_hwaddr(IntPtr tapcfg, IntPtr length) {
				return tapcfg_iface_get_hwaddr(tapcfg, length);
			}

			public override int iface_set_hwaddr(IntPtr tapcfg, byte[] hwaddr, int length) {
				return tapcfg_iface_set_hwaddr(tapcfg, hwaddr, length);
			}

			public override int iface_get_status(IntPtr tapcfg) {
				return tapcfg_iface_get_status(tapcfg);
			}

			public override int iface_set_status(IntPtr tapcfg, int flags) {
				return tapcfg_iface_set_status(tapcfg, flags);
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

			public override int iface_set_dhcp_options(IntPtr tapcfg, byte[] buffer, int buflen) {
				return tapcfg_iface_set_dhcp_options(tapcfg, buffer, buflen);
			}

			[DllImport("tapcfg32")]
			private static extern void tapcfg_set_log_callback(IntPtr tapcfg, InternalLogCallback cb);

			[DllImport("tapcfg32")]
			private static extern IntPtr tapcfg_init();
			[DllImport("tapcfg32")]
			private static extern void tapcfg_destroy(IntPtr tapcfg);

			[DllImport("tapcfg32")]
			private static extern int tapcfg_start(IntPtr tapcfg, IntPtr ifname, int fallback);
			[DllImport("tapcfg32")]
			private static extern void tapcfg_stop(IntPtr tapcfg);

			[DllImport("tapcfg32")]
			private static extern int tapcfg_wait_readable(IntPtr tapcfg, int msec);
			[DllImport("tapcfg32")]
			private static extern int tapcfg_wait_writable(IntPtr tapcfg, int msec);

			[DllImport("tapcfg32")]
			private static extern int tapcfg_read(IntPtr tapcfg, byte[] buf, int count);
			[DllImport("tapcfg32")]
			private static extern int tapcfg_write(IntPtr tapcfg, byte[] buf, int count);

			[DllImport("tapcfg32")]
			private static extern IntPtr tapcfg_get_ifname(IntPtr tapcfg);

			[DllImport("tapcfg32")]
			private static extern IntPtr tapcfg_iface_get_hwaddr(IntPtr tapcfg, IntPtr length);
			[DllImport("tapcfg32")]
			private static extern int tapcfg_iface_set_hwaddr(IntPtr tapcfg, byte[] hwaddr, int length);

			[DllImport("tapcfg32")]
			private static extern int tapcfg_iface_get_status(IntPtr tapcfg);
			[DllImport("tapcfg32")]
			private static extern int tapcfg_iface_set_status(IntPtr tapcfg, int flags);
			[DllImport("tapcfg32")]
			private static extern int tapcfg_iface_get_mtu(IntPtr tapcfg);
			[DllImport("tapcfg32")]
			private static extern int tapcfg_iface_set_mtu(IntPtr tapcfg, int mtu);
			[DllImport("tapcfg32")]
			private static extern int tapcfg_iface_set_ipv4(IntPtr tapcfg, string addr, byte netbits);
			[DllImport("tapcfg32")]
			private static extern int tapcfg_iface_set_dhcp_options(IntPtr tapcfg, byte[] buffer, int buflen);
		}

		private class NativeLib64 : NativeLib {
			public override void set_log_callback(IntPtr tapcfg, LogCallback cb) {
				_logCallback = cb;
				if (_logCallback != null) {
					tapcfg_set_log_callback(tapcfg, new InternalLogCallback(MarshalLogCallback));
				} else {
					tapcfg_set_log_callback(tapcfg, null);
				}
			}

			public override IntPtr init() {
				return tapcfg_init();
			}

			public override void destroy(IntPtr tapcfg) {
				tapcfg_destroy(tapcfg);
			}

			public override int start(IntPtr tapcfg, string ifname, bool fallback) {
				ICustomMarshaler marshaler = new UTF8Marshaler();
				IntPtr ifname_ptr = marshaler.MarshalManagedToNative(ifname);
				int ret = tapcfg_start(tapcfg, ifname_ptr, fallback ? 1 : 0);
				marshaler.CleanUpNativeData(ifname_ptr);
				return ret;
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
				ICustomMarshaler marshaler = new UTF8Marshaler();
				IntPtr ret_ptr = tapcfg_get_ifname(tapcfg);
				string ret = (string) marshaler.MarshalNativeToManaged(ret_ptr);
				marshaler.CleanUpNativeData(ret_ptr);
				return ret;
			}

			public override IntPtr iface_get_hwaddr(IntPtr tapcfg, IntPtr length) {
				return tapcfg_iface_get_hwaddr(tapcfg, length);
			}

			public override int iface_set_hwaddr(IntPtr tapcfg, byte[] hwaddr, int length) {
				return tapcfg_iface_set_hwaddr(tapcfg, hwaddr, length);
			}

			public override int iface_get_status(IntPtr tapcfg) {
				return tapcfg_iface_get_status(tapcfg);
			}

			public override int iface_set_status(IntPtr tapcfg, int flags) {
				return tapcfg_iface_set_status(tapcfg, flags);
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

			public override int iface_set_dhcp_options(IntPtr tapcfg, byte[] buffer, int buflen) {
				return tapcfg_iface_set_dhcp_options(tapcfg, buffer, buflen);
			}

			[DllImport("tapcfg64")]
			private static extern void tapcfg_set_log_callback(IntPtr tapcfg, InternalLogCallback cb);

			[DllImport("tapcfg64")]
			private static extern IntPtr tapcfg_init();
			[DllImport("tapcfg64")]
			private static extern void tapcfg_destroy(IntPtr tapcfg);

			[DllImport("tapcfg64")]
			private static extern int tapcfg_start(IntPtr tapcfg, IntPtr ifname, int fallback);
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
			private static extern IntPtr tapcfg_get_ifname(IntPtr tapcfg);

			[DllImport("tapcfg64")]
			private static extern IntPtr tapcfg_iface_get_hwaddr(IntPtr tapcfg, IntPtr length);
			[DllImport("tapcfg64")]
			private static extern int tapcfg_iface_set_hwaddr(IntPtr tapcfg, byte[] hwaddr, int length);

			[DllImport("tapcfg64")]
			private static extern int tapcfg_iface_get_status(IntPtr tapcfg);
			[DllImport("tapcfg64")]
			private static extern int tapcfg_iface_set_status(IntPtr tapcfg, int flags);
			[DllImport("tapcfg64")]
			private static extern int tapcfg_iface_get_mtu(IntPtr tapcfg);
			[DllImport("tapcfg64")]
			private static extern int tapcfg_iface_set_mtu(IntPtr tapcfg, int mtu);
			[DllImport("tapcfg64")]
			private static extern int tapcfg_iface_set_ipv4(IntPtr tapcfg, string addr, byte netbits);
			[DllImport("tapcfg64")]
			private static extern int tapcfg_iface_set_dhcp_options(IntPtr tapcfg, byte[] buffer, int buflen);
		}
	}
}

