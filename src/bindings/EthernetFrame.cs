
using System;

namespace TAPCfg {
	public class EthernetFrame {
		private byte[] data;
		private byte[] src = new byte[6];
		private byte[] dst = new byte[6];
		private int etherType;

		public EthernetFrame(byte[] data) {
			this.data = data;
			Array.Copy(data, 0, dst, 0, 6);
			Array.Copy(data, 6, src, 0, 6);
			etherType = (data[12] << 8) | data[13];
		}

		public byte[] Data {
			get { return data; }
		}

		public int Length {
			get { return data.Length; }
		}

		public byte[] SourceAddress {
			get { return src; }
		}

		public byte[] DestinationAddress {
			get { return dst; }
		}

		public int EtherType {
			get { return etherType; }
		}

		public byte[] Payload {
			get {
				byte[] ret = new byte[data.Length - 14];
				Array.Copy(data, 14, ret, 0, ret.Length);
				return ret;
			}
		}
	}
}
