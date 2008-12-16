/**
 *  tapcfg - A cross-platform configuration utility for TAP driver
 *  Copyright (C) 2008  Juho Vähä-Herttua
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
using System.Threading;

namespace TAP {
	public interface INetworkHost {
		void HandleFrame(EthernetFrame frame);
	}

	public class LocalHost : INetworkHost {
		private Object _mutex;
		private EthernetDevice _dev;
		private Thread _thread;
		private volatile bool _running;
		private PacketMangler _mangler;

		public LocalHost(EthernetDevice dev, PacketMangler mangler) {
			_mutex = new Object();
			_dev = dev;
			_running = false;
			_mangler = mangler;
		}

		public void Start() {
			_thread = new Thread(new ThreadStart(threadFunc));
			_running = true;
			_thread.Start();
		}

		public void Stop() {
			_running = false;
			_thread.Join();
		}

		private void threadFunc() {
			while (_running) {
				if (_dev.WaitReadable(500)) {
					EthernetFrame frame = _dev.Read();
					_mangler.MangleFrame(this, frame);
				}
			}
		}

		public void HandleFrame(EthernetFrame frame) {
			lock (_mutex) {
				_dev.Write(frame);
			}
		}
	}
}
