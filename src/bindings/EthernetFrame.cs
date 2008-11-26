

namespace TAPCfg {
	public class EthernetFrame {
		private byte[] data;

		public EthernetFrame(byte[] data) {
			this.data = data;
		}

		public byte[] Data {
			get {
				return data;
			}
		}
	}
}
