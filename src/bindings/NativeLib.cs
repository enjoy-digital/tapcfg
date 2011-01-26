/**
 *  tapcfg - A cross-platform configuration utility for TAP driver
 *  Copyright (C) 2008-2011  Juho Vähä-Herttua
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
		Emergency  = 0,
		Alert      = 1,
		Critical   = 2,
		Error      = 3,
		Warning    = 4,
		Notice     = 5,
		Info       = 6,
		Debug      = 7,
		Unknown    = 255
	};
	public delegate void LogCallback(LogLevel level, string msg);

	public abstract class NativeLib {
		public abstract int get_version();
		public abstract void set_log_level(IntPtr tapcfg, LogLevel logLevel);
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
		public abstract int iface_set_ipv6(IntPtr tapcfg, string addr, byte netbits);
		public abstract int iface_set_dhcp_options(IntPtr tapcfg, byte[] buffer, int buflen);
		public abstract int iface_set_dhcpv6_options(IntPtr tapcfg, byte[] buffer, int buflen);

		public static NativeLib GetInstance() {
			if (IntPtr.Size == 8)
				return new NativeLib64();
			else
				return new NativeLib32();
		}

		private LogCallback _logCallback = null;
		private InternalLogCallback _internalLogCallback = null;
		ICustomMarshaler _utf8Marshaler = new UTF8Marshaler();

		[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
		private delegate void InternalLogCallback(int level, IntPtr msg_ptr);
		private void MarshalLogCallback(int level, IntPtr msg_ptr) {
			object value = level;
			LogLevel logLevel;

			if (Enum.IsDefined(typeof(LogLevel), value)) {
				logLevel = (LogLevel) Enum.ToObject(typeof(LogLevel), value);
			} else {
				logLevel = LogLevel.Unknown;
			}

			string msg = (string) _utf8Marshaler.MarshalNativeToManaged(msg_ptr);
			_logCallback(logLevel, msg);
		}

		private class NativeLib32 : NativeLib {
			public NativeLib32() {
				_internalLogCallback = new InternalLogCallback(MarshalLogCallback);
			}

			public override int get_version() {
				return tapcfg_get_version();
			}

			public override void set_log_level(IntPtr tapcfg, LogLevel logLevel) {
				int level = (int) logLevel;
				tapcfg_set_log_level(tapcfg, level);
			}

			public override void set_log_callback(IntPtr tapcfg, LogCallback cb) {
				_logCallback = cb;
				if (_logCallback != null) {
					tapcfg_set_log_callback(tapcfg, _internalLogCallback);
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
				IntPtr ifname_ptr = _utf8Marshaler.MarshalManagedToNative(ifname);
				int ret = tapcfg_start(tapcfg, ifname_ptr, fallback ? 1 : 0);
				_utf8Marshaler.CleanUpNativeData(ifname_ptr);
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
				IntPtr ret_ptr = tapcfg_get_ifname(tapcfg);
				return (string) _utf8Marshaler.MarshalNativeToManaged(ret_ptr);
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

			public override int iface_set_ipv6(IntPtr tapcfg, string addr, byte netbits) {
				return tapcfg_iface_set_ipv6(tapcfg, addr, netbits);
			}

			public override int iface_set_dhcp_options(IntPtr tapcfg, byte[] buffer, int buflen) {
				return tapcfg_iface_set_dhcp_options(tapcfg, buffer, buflen);
			}

			public override int iface_set_dhcpv6_options(IntPtr tapcfg, byte[] buffer, int buflen) {
				return tapcfg_iface_set_dhcpv6_options(tapcfg, buffer, buflen);
			}

			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_get_version();
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern void tapcfg_set_log_level(IntPtr tapcfg, int level);
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern void tapcfg_set_log_callback(IntPtr tapcfg, InternalLogCallback cb);

			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern IntPtr tapcfg_init();
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern void tapcfg_destroy(IntPtr tapcfg);

			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_start(IntPtr tapcfg, IntPtr ifname, int fallback);
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern void tapcfg_stop(IntPtr tapcfg);

			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_wait_readable(IntPtr tapcfg, int msec);
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_wait_writable(IntPtr tapcfg, int msec);

			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_read(IntPtr tapcfg, byte[] buf, int count);
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_write(IntPtr tapcfg, byte[] buf, int count);

			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern IntPtr tapcfg_get_ifname(IntPtr tapcfg);

			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern IntPtr tapcfg_iface_get_hwaddr(IntPtr tapcfg, IntPtr length);
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_hwaddr(IntPtr tapcfg, byte[] hwaddr, int length);

			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_get_status(IntPtr tapcfg);
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_status(IntPtr tapcfg, int flags);
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_get_mtu(IntPtr tapcfg);
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_mtu(IntPtr tapcfg, int mtu);
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_ipv4(IntPtr tapcfg, string addr, byte netbits);
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_ipv6(IntPtr tapcfg, string addr, byte netbits);
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_dhcp_options(IntPtr tapcfg, byte[] buffer, int buflen);
			[DllImport("tapcfg32", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_dhcpv6_options(IntPtr tapcfg, byte[] buffer, int buflen);
		}

		private class NativeLib64 : NativeLib {
			public NativeLib64() {
				_internalLogCallback = new InternalLogCallback(MarshalLogCallback);
			}

			public override int get_version() {
				return tapcfg_get_version();
			}

			public override void set_log_level(IntPtr tapcfg, LogLevel logLevel) {
				int level = (int) logLevel;
				tapcfg_set_log_level(tapcfg, level);
			}

			public override void set_log_callback(IntPtr tapcfg, LogCallback cb) {
				_logCallback = cb;
				if (_logCallback != null) {
					tapcfg_set_log_callback(tapcfg, _internalLogCallback);
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
				IntPtr ifname_ptr = _utf8Marshaler.MarshalManagedToNative(ifname);
				int ret = tapcfg_start(tapcfg, ifname_ptr, fallback ? 1 : 0);
				_utf8Marshaler.CleanUpNativeData(ifname_ptr);
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
				IntPtr ret_ptr = tapcfg_get_ifname(tapcfg);
				return (string) _utf8Marshaler.MarshalNativeToManaged(ret_ptr);
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

			public override int iface_set_ipv6(IntPtr tapcfg, string addr, byte netbits) {
				return tapcfg_iface_set_ipv6(tapcfg, addr, netbits);
			}

			public override int iface_set_dhcp_options(IntPtr tapcfg, byte[] buffer, int buflen) {
				return tapcfg_iface_set_dhcp_options(tapcfg, buffer, buflen);
			}

			public override int iface_set_dhcpv6_options(IntPtr tapcfg, byte[] buffer, int buflen) {
				return tapcfg_iface_set_dhcpv6_options(tapcfg, buffer, buflen);
			}

			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_get_version();
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern void tapcfg_set_log_level(IntPtr tapcfg, int level);
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern void tapcfg_set_log_callback(IntPtr tapcfg, InternalLogCallback cb);

			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern IntPtr tapcfg_init();
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern void tapcfg_destroy(IntPtr tapcfg);

			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_start(IntPtr tapcfg, IntPtr ifname, int fallback);
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern void tapcfg_stop(IntPtr tapcfg);

			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_wait_readable(IntPtr tapcfg, int msec);
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_wait_writable(IntPtr tapcfg, int msec);

			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_read(IntPtr tapcfg, byte[] buf, int count);
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_write(IntPtr tapcfg, byte[] buf, int count);

			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern IntPtr tapcfg_get_ifname(IntPtr tapcfg);

			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern IntPtr tapcfg_iface_get_hwaddr(IntPtr tapcfg, IntPtr length);
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_hwaddr(IntPtr tapcfg, byte[] hwaddr, int length);

			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_get_status(IntPtr tapcfg);
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_status(IntPtr tapcfg, int flags);
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_get_mtu(IntPtr tapcfg);
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_mtu(IntPtr tapcfg, int mtu);
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_ipv4(IntPtr tapcfg, string addr, byte netbits);
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_ipv6(IntPtr tapcfg, string addr, byte netbits);
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_dhcp_options(IntPtr tapcfg, byte[] buffer, int buflen);
			[DllImport("tapcfg64", CallingConvention=CallingConvention.Cdecl)]
			private static extern int tapcfg_iface_set_dhcpv6_options(IntPtr tapcfg, byte[] buffer, int buflen);
		}
	}
}

