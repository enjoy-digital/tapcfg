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

namespace TAP {
	public delegate void EthernetLogCallback(
		[MarshalAs(UnmanagedType.CustomMarshaler,
			   MarshalTypeRef = typeof(UTF8Marshaler))]
		string msg);

	public class EthernetDevice : IDisposable {
		private NativeLib _tapcfg;

		private IntPtr _handle;
		private bool _disposed = false;

		private static EthernetLogCallback _logger;
		public static EthernetLogCallback LogCallback {
			set {
				_logger = value;
				NativeLib.GetInstance().set_callback(_logger);
			}
		}

		private static void defaultCallback(string msg) {
			Console.WriteLine(msg);
		}

		public EthernetDevice() {
			if (_logger == null) {
				EthernetDevice.LogCallback = new EthernetLogCallback(defaultCallback);
			}

			_tapcfg = NativeLib.GetInstance();
			_handle = _tapcfg.init();
			if (_handle == IntPtr.Zero) {
				throw new Exception("Error initializing the tapcfg library");
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

		public bool WaitReadable(int msec) {
			int ret = _tapcfg.wait_readable(_handle, msec);
			return (ret != 0);
		}

		public EthernetFrame Read() {
			/* Maximum buffer is MTU plus 22 byte maximum header size */
			byte[] buffer = new byte[1500 + 22];

			int ret = _tapcfg.read(_handle, buffer, buffer.Length);
			if (ret < 0) {
				throw new IOException("Error reading Ethernet frame");
			} else if (ret == 0) {
				throw new EndOfStreamException("Unexpected EOF");
			}

			return new EthernetFrame(buffer);
		}

		public bool WaitWritable(int msec) {
			int ret = _tapcfg.wait_writable(_handle, msec);
			return (ret != 0);
		}

		public void Write(EthernetFrame frame) {
			byte[] buffer = frame.Data;

			int ret = _tapcfg.write(_handle, buffer, buffer.Length);
			if (ret < 0) {
				throw new IOException("Error writing Ethernet frame");
			} else if (ret != buffer.Length) {
				/* This shouldn't be possible, writes are blocking */
				throw new IOException("Incomplete write writing Ethernet frame");
			}
		}

		public bool Enabled {
			get {
				int ret = _tapcfg.iface_get_status(_handle);
				return (ret != 0);
			}
			set {
				int ret;

				if (value) {
					ret = _tapcfg.iface_change_status(_handle, 1);
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
				IntPtr lenptr = Marshal.AllocHGlobal(10);

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
