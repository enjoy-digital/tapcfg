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

#include <sys/sockio.h>
#include <sys/stropts.h>

#define TUNNEWPPA       (('T'<<16) | 0x0001)
#define TUNSETPPA       (('T'<<16) | 0x0002)

static int
tapcfg_start_dev(tapcfg_t *tapcfg, const char *ifname, int fallback)
{
	int tap_fd = -1;
	struct lifreq lifr;
	struct strioctl strioc;
	int ppa, newppa;

	if (strncmp(ifname, "tap", 3)) {
		if (!fallback) {
			taplog_log(TAPLOG_DEBUG,
				   "Device name '%s' doesn't start with 'tap'\n",
				   ifname);
			return -1;
		}
		ppa = 0;
	} else {
		char *endptr;

		ppa = strtol(ifname+3, &endptr, 10);
		if (*endptr != '\0') {
			if (!fallback)
				return -1;
			ppa = 0;
		}
	}

	tap_fd = open("/dev/tap", O_RDWR, 0);
	if (tap_fd < 0) {
		taplog_log(TAPLOG_ERR,
			   "Couldn't open the tap device\n");
		taplog_log(TAPLOG_INFO,
			   "Check that you are running the program with "
			   "root privileges and have TUN/TAP driver installed\n");
		return -1;
	}

	strioc.ic_cmd = TUNNEWPPA;
	strioc.ic_timout = 0;
	strioc.ic_len = sizeof(ppa);
	strioc.ic_dp = (char *) &ppa;
	newppa = ioctl(tap_fd, I_STR, &strioc);
	if (newppa == -1 && !fallback) {
		taplog_log(TAPLOG_ERR,
		           "Couldn't assign new interface: tap%d\n",
		           ppa);
		close(tap_fd);
		return -1;
	} else if (newppa == -1) {
		int i;

		taplog_log(TAPLOG_INFO,
		           "Opening device '%s' failed, trying to find another one\n",
		           ifname);
		for (i=0; i<16; i++) {
			if (i == ppa)
				continue;

			strioc.ic_cmd = TUNNEWPPA;
			strioc.ic_timout = 0;
			strioc.ic_len = sizeof(i);
			strioc.ic_dp = (char *) &i;
			newppa = ioctl(tap_fd, I_STR, &strioc);
			if (newppa >= 0) {
				/* Found one! */
				break;
			}
		}
		if (ppa == 16) {
			taplog_log(TAPLOG_ERR,
			           "Couldn't find suitable tap device to assign\n");
			return -1;
		}
	}

	/* Set the device name to be the one we found finally */
	snprintf(tapcfg->ifname,
	         sizeof(tapcfg->ifname)-1,
	         "tap%d", newppa);
	taplog_log(TAPLOG_DEBUG, "Device name %s\n", tapcfg->ifname);

	if (ioctl(tap_fd, I_PUSH, "ip") == -1) {
		taplog_log(TAPLOG_ERR, "Error pushing the IP module\n");
		close(tap_fd);
		return -1;
	}

	memset(&lifr, 0, sizeof(struct lifreq));
	if (ioctl(tap_fd, SIOCGLIFFLAGS, &lifr) == -1) {
		taplog_log(TAPLOG_ERR, "Can't get interface flags\n");
		close(tap_fd);
		return -1;
	}

	strcpy(lifr.lifr_name, tapcfg->ifname);
	lifr.lifr_ppa = ppa;
	if (ioctl(tap_fd, SIOCSLIFNAME, &lifr) == -1) {
		taplog_log(TAPLOG_ERR, "Couldn't set interface name\n");
		close(tap_fd);
		return -1;
	}

	if (ioctl(tap_fd, I_PUSH, "arp") == -1) {
		taplog_log(TAPLOG_ERR, "Error pushing the ARP module\n");
		close(tap_fd);
		return -1;
	}

	/* XXX: Get MAC address on Solaris, need to use DLIP... */

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
