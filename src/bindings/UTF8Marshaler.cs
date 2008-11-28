
using System;
using System.Text;
using System.Collections;
using System.Runtime.InteropServices;

namespace TAP {
	public class UTF8Marshaler : ICustomMarshaler {
		static UTF8Marshaler marshaler = new UTF8Marshaler();

		private Hashtable allocated = new Hashtable();

		public static ICustomMarshaler GetInstance(string cookie) {
			return marshaler;
		}

		public void CleanUpManagedData(object ManagedObj) {
		}

		public void CleanUpNativeData(IntPtr pNativeData) {
			/* This is a hack not to crash on mono!!! */
			if (allocated.Contains(pNativeData)) {
				Marshal.FreeHGlobal(pNativeData);
				allocated.Remove(pNativeData);
			} else {
				Console.WriteLine("WARNING: Trying to free an unallocated pointer!");
				Console.WriteLine("         This is most likely a bug in mono");
			}
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

			/* This is a hack not to crash on mono!!! */
			allocated.Add(ptr, null);

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
