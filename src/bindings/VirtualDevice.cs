/**
 *  tapcfg - A cross-platform configuration utility for TAP driver
 *  Copyright (C) 2008-2009  Juho Vähä-Herttua
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
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;

namespace TAPNet {
	public delegate void TAPLogCallback(
		[MarshalAs(UnmanagedType.CustomMarshaler,
			   MarshalTypeRef = typeof(UTF8Marshaler))]
		string msg);

	public class VirtualDevice : IDisposable {
		private NativeLib _tapcfg;

		private IntPtr _handle;
		private bool _disposed = false;

		private static void defaultCallback(string msg) {
			Console.WriteLine(msg);
		}

		public VirtualDevice() {
			_tapcfg = NativeLib.GetInstance();
			_handle = _tapcfg.init();
			if (_handle == IntPtr.Zero) {
				throw new Exception("Error initializing the tapcfg library");
			}

			LogCallback = new TAPLogCallback(defaultCallback);
		}

		public TAPLogCallback LogCallback {
			set {
				_tapcfg.set_log_callback(_handle, value);
			}
		}

		public void Start() {
			Start(null, true);
		}

		public void Start(string deviceName, bool fallback) {
			int ret = _tapcfg.start(_handle, deviceName, fallback);
			if (ret < 0) {
				throw new Exception("Error starting the TAP device");
			}
		}

		public void Stop() {
			_tapcfg.stop(_handle);
		}

		public bool WaitReadable(int msec) {
			int ret = _tapcfg.wait_readable(_handle, msec);
			return (ret != 0);
		}

		public int ReadTo(byte[] buffer) {
			int ret = _tapcfg.read(_handle, buffer, buffer.Length);
			if (ret < 0) {
				throw new IOException("Error reading frame");
			} else if (ret == 0) {
				throw new EndOfStreamException("Unexpected EOF");
			}

			return ret;
		}

		public byte[] Read() {
			byte[] buffer = new byte[4096];

			int len = ReadTo(buffer);
			byte[] ret = new byte[len];
			Array.Copy(buffer, 0, ret, 0, len);

			return ret;
		}

		public bool WaitWritable(int msec) {
			int ret = _tapcfg.wait_writable(_handle, msec);
			return (ret != 0);
		}

		public void WriteFrom(byte[] buffer, int length) {
			int ret = _tapcfg.write(_handle, buffer, length);
			if (ret < 0) {
				throw new IOException("Error writing frame");
			} else if (ret != buffer.Length) {
				/* This shouldn't be possible, writes are blocking */
				throw new IOException("Incomplete write when writing frame");
			}
		}

		public void Write(byte[] buffer) {
			WriteFrom(buffer, buffer.Length);
		}

		public bool Enabled {
			get {
				int ret = _tapcfg.iface_get_status(_handle);
				return (ret != 0);
			}
			set {
				int ret;

				if (value) {
					ret = _tapcfg.iface_change_status(_handle, 0xffff);
				} else {
					ret = _tapcfg.iface_change_status(_handle, 0);
				}

				if (ret < 0) {
					throw new Exception("Error changing TAP interface status");
				}
			}
		}

		public string DeviceName {
			get { return _tapcfg.get_ifname(_handle); }
		}

		public byte[] HWAddress {
			get {
				/* Allocate unmanaged memory to store the returned array length */
				IntPtr lenptr = Marshal.AllocHGlobal(8);

				/* Call the function, length of the data array is stored in lenptr */
				IntPtr data = _tapcfg.iface_get_hwaddr(_handle, lenptr);

				/* Read the array length into a managed value */
				int datalen = Marshal.ReadInt32(lenptr, 0);
				Marshal.FreeHGlobal(lenptr);

				/* Copy the data into a managed array */
				byte[] ret = new byte[datalen];
				Marshal.Copy(data, ret, 0, datalen);

				return ret;
			}
			set {
				_tapcfg.iface_set_hwaddr(_handle, value, value.Length);
				/* XXX: Is it ok to ignore HWAddress setting failure */
			}
		}

		public int MTU {
			get {
				int ret = _tapcfg.iface_get_mtu(_handle);
				if (ret < 0) {
					throw new Exception("Error getting TAP interface MTU");
				}
				return ret;
			}
			set {
				_tapcfg.iface_set_mtu(_handle, value);
				/* XXX: Is it ok to ignore MTU setting failure */
			}
		}

		public void SetAddress(IPAddress address, byte netbits) {
			int ret;

			if (address.AddressFamily == AddressFamily.InterNetwork) {
				ret = _tapcfg.iface_set_ipv4(_handle, address.ToString(), netbits);
			} else {
				return;
			}

			if (ret < 0) {
				throw new Exception("Error setting IP address: " + address.ToString());
			}
		}

		public void SetDHCPOptions(byte[] options) {
			_tapcfg.iface_set_dhcp_options(_handle, options, options.Length);
			/* XXX: Is it ok to ignore DHCP options setting failure */
		}

		public void Dispose() {
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing) {
			if (!_disposed) {
				if (disposing) {
					// Managed resources can be disposed here
				}

				_tapcfg.stop(_handle);
				_tapcfg.destroy(_handle);
				_handle = IntPtr.Zero;

				_disposed = true;
			}
		}
	}
}
