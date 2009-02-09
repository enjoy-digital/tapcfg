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

	/* Get the hardware address of the TAP interface */
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, tapcfg->ifname);
	ret = ioctl(tap_fd, SIOCGIFHWADDR, &ifr);
	if (ret == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error getting the hardware address: %s\n",
		           strerror(errno));
		return -1;
	}
	memcpy(tapcfg->hwaddr, ifr.ifr_hwaddr.sa_data, IFHWADDRLEN);
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

	/* FIXME: Get MAC address on BSD, slightly tricky */
#endif

	return tap_fd;
}

/* This is to accept router advertisements on KAME stack, these
 * functions are copied from usr.sbin/rtsold/if.c of OpenBSD */
#if defined(IPV6CTL_FORWARDING) && defined(IPV6CTL_ACCEPT_RTADV)
#include <sys/sysctl.h>

static int
getinet6sysctl(int code)
{
	int mib[] = { CTL_NET, PF_INET6, IPPROTO_IPV6, 0 };
	int value;
	size_t size;

	mib[3] = code;
	size = sizeof(value);
	if (sysctl(mib, sizeof(mib)/sizeof(mib[0]), &value, &size, NULL, 0) < 0)
		return -1;
	else
		return value;
}

static int
setinet6sysctl(int code, int newval)
{
	int mib[] = { CTL_NET, PF_INET6, IPPROTO_IPV6, 0 };
	int value;
	size_t size;

	mib[3] = code;
	size = sizeof(value);
	if (sysctl(mib, sizeof(mib)/sizeof(mib[0]), &value, &size,
	    &newval, sizeof(newval)) < 0)
		return -1;
	else
		return value;
}
#endif

#ifdef __APPLE__
#include <netinet/in_var.h>

/* This function is based on ip6tool.c of Darwin sources */
static int
tapcfg_attach_ipv6(const char *ifname)
{
	struct in6_aliasreq ifr;
	int s, err;

	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifra_name, ifname, sizeof(ifr.ifra_name));

	/* Attach IPv6 protocol to the interface */
	if ((err = ioctl(s, SIOCPROTOATTACH_IN6, &ifr)) != 0)
		return -1;

	/* Start acquiring linklocal address on the interface */
	if ((err = ioctl(s, SIOCLL_START, &ifr)) != 0)
		return -1;

	close(s);

	return 0;
}
#endif

static void
tapcfg_iface_prepare(const char *ifname)
{
#if defined(IPV6CTL_FORWARDING) && defined(IPV6CTL_ACCEPT_RTADV)
	if (getinet6sysctl(IPV6CTL_FORWARDING) == 1) {
		taplog_log(TAPLOG_INFO,
		           "Setting sysctl net.inet6.ip6.forwarding: 1 -> 0\n");
		setinet6sysctl(IPV6CTL_FORWARDING, 0);
	}
	if (getinet6sysctl(IPV6CTL_ACCEPT_RTADV) == 0) {
		taplog_log(TAPLOG_INFO,
		           "Setting sysctl net.inet6.ip6.accept_rtadv: 0 -> 1\n");
		setinet6sysctl(IPV6CTL_ACCEPT_RTADV, 1);
	}
#endif

#ifdef __APPLE__
	tapcfg_attach_ipv6(ifname);
#endif
}

