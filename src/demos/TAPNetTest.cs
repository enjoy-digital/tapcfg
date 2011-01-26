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

using TAPNet;
using System;
using System.Net;
using System.Threading;

public class TAPNetTest {
	private static void Main(string[] args) {
		VirtualDevice dev = new VirtualDevice();
		dev.LogCallback = new LogCallback(LogCallback);
		dev.Start("Device name", true);
		Console.WriteLine("Got device name: {0}", dev.DeviceName);
		Console.WriteLine("Got device hwaddr: {0}", BitConverter.ToString(dev.HWAddress));
		dev.HWAddress = new byte[] { 0x00, 0x01, 0x23, 0x45, 0x67, 0x89 };
		dev.MTU = 1280;
		dev.SetAddress(IPAddress.Parse("192.168.10.1"), 16);
		dev.Enabled = true;

		while (true) {
			Thread.Sleep(1000);
		}
	}

	private static void LogCallback(LogLevel level, string msg) {
		Console.WriteLine(level + ": " + msg);
	}
}
