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

#ifdef __linux__
#  include <linux/if_tun.h>
#endif

static int
tapcfg_start_dev(tapcfg_t *tapcfg, const char *ifname)
{
	int tap_fd = -1;
#ifdef __linux__
	struct ifreq ifr;
	int ret;

	/* Create a new tap device */
	tap_fd = open("/dev/net/tun", O_RDWR);
	if (tap_fd == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error opening device /dev/net/tun: %s\n",
		           strerror(errno));
		taplog_log(TAPLOG_INFO,
		           "Check that you are running the program with "
		           "root privileges\n");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	if (ifname && strlen(ifname) < IFNAMSIZ) {
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);
	}
	ret = ioctl(tap_fd, TUNSETIFF, &ifr);

	if (ret == -1 && errno == EINVAL) {
		/* Try again without device name */
		memset(&ifr, 0, sizeof(ifr));
		ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
		ret = ioctl(tap_fd, TUNSETIFF, &ifr);
	}
	if (ret == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error setting the interface: %s\n",
		           strerror(errno));
		return -1;
	}

	/* Set the device name to be the one we got from OS */
	taplog_log(TAPLOG_DEBUG, "Device name %s\n", ifr.ifr_name);
	strncpy(tapcfg->ifname, ifr.ifr_name, sizeof(tapcfg->ifname)-1);
#else /* BSD */
	char buf[128];
	int i;

	buf[sizeof(buf)-1] = '\0';

	/* If we have a configured interface name, try that first */
	if (ifname && strlen(ifname) <= MAX_IFNAME && !strrchr(ifname, ' ')) {
		snprintf(buf, sizeof(buf)-1, "/dev/%s", ifname);
		tap_fd = open(buf, O_RDWR);
	}
	if (tap_fd < 0) {
		/* Try all possible devices, because configured name failed */
		for (i=0; i<16; i++) {
			snprintf(buf, sizeof(buf)-1, "/dev/tap%u", i);
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
#endif

	return tap_fd;
}

