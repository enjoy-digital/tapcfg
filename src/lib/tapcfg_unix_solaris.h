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

static int
tapcfg_start_dev(tapcfg_t *tapcfg, const char *ifname, int fallback)
{
	int tap_fd = -1;
	char buf[128];
	struct ifreq ifr;
	int i, ret;

	buf[sizeof(buf)-1] = '\0';

	/* If we have a configured interface name, try that first */
	if (ifname && strlen(ifname) <= MAX_IFNAME && !strrchr(ifname, ' ')) {
		snprintf(buf, sizeof(buf)-1, "/dev/%s", ifname);
		tap_fd = open(buf, O_RDWR);
	}
	if (tap_fd < 0 && !fallback) {
		taplog_log(TAPLOG_ERR,
			   "Couldn't open the tap device \"%s\"\n", ifname);
		taplog_log(TAPLOG_INFO,
			   "Check that you are running the program with "
			   "root privileges and have TUN/TAP driver installed\n");
		return -1;
	} else {
		/* Try all possible devices, because configured name failed */
		for (i=-1; i<16; i++) {
			if (i < 0) {
				snprintf(buf, sizeof(buf)-1, "/dev/tap");
			} else {
				snprintf(buf, sizeof(buf)-1, "/dev/tap%u", i);
			}
			tap_fd = open(buf, O_RDWR);
			if (tap_fd >= 0) {
				/* Found one! Could save this for later... */
				break;
			}
		}
		if (i == 16) {
			taplog_log(TAPLOG_ERR,
				   "Couldn't find a suitable tap device\n");
			taplog_log(TAPLOG_INFO,
				   "Check that you are running the program with "
				   "root privileges and have TUN/TAP driver installed\n");
			return -1;
		}
	}

	/* Set the device name to be the one we found finally */
	taplog_log(TAPLOG_DEBUG, "Device name %s\n", buf+5);
	strncpy(tapcfg->ifname, buf+5, sizeof(tapcfg->ifname)-1);

	/* Get MAC address on Solaris */
	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, tapcfg->ifname);
	ret = ioctl(tap_fd, SIOCGENADDR, &ifr);
	if (ret == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error getting the MAC address of TAP device '%s': %s\n",
		           tapcfg->ifname, strerror(errno));
	}
	memcpy(tapcfg->hwaddr, ifr.ifr_enaddr, HWADDRLEN);

	return tap_fd;
}

static void
tapcfg_iface_prepare(const char *ifname)
{
}

static int
tapcfg_hwaddr_ioctl(int ctrl_fd,
                    const char *ifname,
                    const char *hwaddr)
{
	return -1;
}

static int
tapcfg_ifaddr_ioctl(int ctrl_fd,
                    const char *ifname,
                    unsigned int addr,
                    unsigned int mask)
{
	struct ifreq ifr;
	struct sockaddr_in *sin;

	memset(&ifr,  0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, ifname);

	sin = (struct sockaddr_in *) &ifr.ifr_addr;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = addr;

	if (ioctl(ctrl_fd, SIOCSIFADDR, &ifr) == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error trying to configure IPv4 address: %s\n",
		           strerror(errno));
		return -1;
	}

	memset(&ifr,  0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, ifname);

	sin = (struct sockaddr_in *) &ifr.ifr_addr;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = mask;

	if (ioctl(ctrl_fd, SIOCSIFNETMASK, &ifr) == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error trying to configure IPv4 netmask: %s\n",
		           strerror(errno));
		return -1;
	}

	return 0;
}
