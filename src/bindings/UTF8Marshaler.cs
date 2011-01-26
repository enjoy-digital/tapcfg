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
using System.Text;
using System.Runtime.InteropServices;

namespace TAPNet {
	public class UTF8Marshaler : ICustomMarshaler {
		static UTF8Marshaler marshaler = new UTF8Marshaler();

		public static ICustomMarshaler GetInstance(string cookie) {
			return marshaler;
		}

		public void CleanUpManagedData(object ManagedObj) {
		}

		public void CleanUpNativeData(IntPtr pNativeData) {
			Marshal.FreeHGlobal(pNativeData);
		}

		public int GetNativeDataSize() {
			return -1;
		}

		public IntPtr MarshalManagedToNative(object ManagedObj) {
			if (ManagedObj == null)
				return IntPtr.Zero;
			if (ManagedObj.GetType() != typeof(string))
				throw new ArgumentException("ManagedObj", "Can only marshal type of System.string");

			byte[] array = Encoding.UTF8.GetBytes((string) ManagedObj);
			int size = Marshal.SizeOf(typeof(byte)) * (array.Length + 1);

			IntPtr ptr = Marshal.AllocHGlobal(size);
			Marshal.Copy(array, 0, ptr, array.Length);
			Marshal.WriteByte(ptr, array.Length, 0);

			return ptr;
		}

		public object MarshalNativeToManaged(IntPtr pNativeData) {
			if (pNativeData == IntPtr.Zero)
				return null;

			int size = 0;
			while (Marshal.ReadByte(pNativeData, size) > 0)
				size++;

			byte[] array = new byte[size];
			Marshal.Copy(pNativeData, array, 0, size);

			return Encoding.UTF8.GetString(array);
		}
	}
}
