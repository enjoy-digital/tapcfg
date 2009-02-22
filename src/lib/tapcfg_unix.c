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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>

/* This is for IFNAMSIZ and ifreq for linux */
#include <net/if.h>

#include "tapcfg.h"
#include "taplog.h"

#define MAX_IFNAME (IFNAMSIZ-1)
#define HWADDRLEN 6

struct tapcfg_s {
	int started;
	int enabled;

	int tap_fd;
	int ctrl_fd;
	char ifname[MAX_IFNAME+1];
	unsigned char hwaddr[HWADDRLEN];

	char buffer[TAPCFG_BUFSIZE];
	int buflen;
};

/* This will use the tapcfg_s struct so we need it here */
#ifdef __linux__
#  include "tapcfg_unix_linux.h"
#else
#  include "tapcfg_unix_bsd.h"
#endif

tapcfg_t *
tapcfg_init()
{
	tapcfg_t *tapcfg;

	tapcfg = calloc(1, sizeof(tapcfg_t));
	if (!tapcfg)
		return NULL;

	tapcfg->tap_fd = -1;

	return tapcfg;
}

void
tapcfg_destroy(tapcfg_t *tapcfg)
{
	if (tapcfg) {
		tapcfg_stop(tapcfg);
	}
	free(tapcfg);
}

int
tapcfg_start(tapcfg_t *tapcfg, const char *ifname, int fallback)
{
	int tap_fd;
	int ctrl_fd;

	assert(tapcfg);

	/* Check if we are already running and return success */
	if (tapcfg->started) {
		return 0;
	}

	tap_fd = tapcfg_start_dev(tapcfg, ifname, fallback);
	if (tap_fd < 0) {
		goto err;
	}

	ctrl_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (ctrl_fd == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error opening control socket for ioctls: %s\n",
		           strerror(errno));
		return -1;
	}


	/* Mark the current fds and mark thread as running */
	tapcfg->tap_fd = tap_fd;
	tapcfg->ctrl_fd = ctrl_fd;
	tapcfg->started = 1;
	tapcfg->enabled = 0;

	return 0;

err:
	/* Clean up in case of an error */
	tapcfg->ifname[0] = '\0';
	if (tap_fd != -1)
		close(tap_fd);
	tapcfg->tap_fd = -1;

	return -1;
}

void
tapcfg_stop(tapcfg_t *tapcfg)
{
	assert(tapcfg);

	if (tapcfg->started) {
		if (tapcfg->tap_fd != -1) {
			close(tapcfg->tap_fd);
			tapcfg->tap_fd = -1;
			close(tapcfg->ctrl_fd);
			tapcfg->ctrl_fd = -1;
		}
		tapcfg->started = 0;
		tapcfg->enabled = 0;
	}
}

int
tapcfg_wait_readable(tapcfg_t *tapcfg, int msec)
{
	fd_set rfds;
	struct timeval tv;
	int ret;

	assert(tapcfg);

	if (!tapcfg->started) {
		return 0;
	}

	tv.tv_sec = (msec / 1000);
	tv.tv_usec = (msec % 1000) * 1000;

	FD_ZERO(&rfds);
	FD_SET(tapcfg->tap_fd, &rfds);
	ret = select(tapcfg->tap_fd+1, &rfds, NULL, NULL, &tv);
	if (ret == -1) {
		/* Error selecting, no data available */
		return 0;
	}
	ret = FD_ISSET(tapcfg->tap_fd, &rfds);

	return ret;
}

int
tapcfg_read(tapcfg_t *tapcfg, void *buf, int count)
{
	int ret;

	assert(tapcfg);

	if (!tapcfg->started) {
		return -1;
	}

	if (!tapcfg->buflen) {
		ret = read(tapcfg->tap_fd, tapcfg->buffer,
			   sizeof(tapcfg->buffer));
		if (ret <= 0) {
			return ret;
		}
		tapcfg->buflen = ret;
	}

	if (count < tapcfg->buflen) {
		taplog_log(TAPLOG_ERR,
		           "Buffer not big enough for reading, "
		           "need at least %d bytes\n",
		           tapcfg->buflen);
		return -1;
	}

	ret = tapcfg->buflen;
	memcpy(buf, tapcfg->buffer, tapcfg->buflen);
	tapcfg->buflen = 0;

	taplog_log(TAPLOG_DEBUG, "Read ethernet frame:\n");
	taplog_log_ethernet_info(buf, ret);

	return ret;
}

int
tapcfg_wait_writable(tapcfg_t *tapcfg, int msec)
{
	fd_set wfds;
	struct timeval tv;
	int ret;

	assert(tapcfg);

	if (!tapcfg->started) {
		return 0;
	}

	tv.tv_sec = (msec / 1000);
	tv.tv_usec = (msec % 1000) * 1000;

	FD_ZERO(&wfds);
	FD_SET(tapcfg->tap_fd, &wfds);
	ret = select(tapcfg->tap_fd+1, NULL, &wfds, NULL, &tv);
	if (ret == -1) {
		/* Error selecting, no data available */
		return 0;
	}
	ret = FD_ISSET(tapcfg->tap_fd, &wfds);

	return ret;
}

int
tapcfg_write(tapcfg_t *tapcfg, void *buf, int count)
{
	int ret;

	assert(tapcfg);

	if (!tapcfg->started) {
		return -1;
	}

	ret = write(tapcfg->tap_fd, buf, count);
	if (ret != count) {
		taplog_log(TAPLOG_ERR,
		           "Error trying to write data to TAP device\n");
		return -1;
	}

	taplog_log(TAPLOG_DEBUG, "Wrote ethernet frame:\n");
	taplog_log_ethernet_info(buf, ret);

	return ret;
}

char *
tapcfg_get_ifname(tapcfg_t *tapcfg)
{
	assert(tapcfg);

	if (!tapcfg->started) {
		return NULL;
	}

	return strdup(tapcfg->ifname);
}

const char *
tapcfg_iface_get_hwaddr(tapcfg_t *tapcfg, int *length)
{
	assert(tapcfg);

	if (!tapcfg->started) {
		return NULL;
	}

	if (length)
		*length = sizeof(tapcfg->hwaddr);
	return (const char *) tapcfg->hwaddr;
}

int
tapcfg_iface_get_status(tapcfg_t *tapcfg)
{
	assert(tapcfg);

	return tapcfg->enabled;
}

int
tapcfg_iface_change_status(tapcfg_t *tapcfg, int enabled)
{
	struct ifreq ifr;

	assert(tapcfg);

	if (!tapcfg->started ||
	    enabled == tapcfg->enabled) {
		/* No need for change, this is ok */
		return 0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, tapcfg->ifname);
	if (ioctl(tapcfg->ctrl_fd, SIOCGIFFLAGS, &ifr) == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error calling SIOCGIFFLAGS for interface %s: %s\n",
		           tapcfg->ifname,
		           strerror(errno));
		return -1;
	}

	if (enabled) {
		ifr.ifr_flags |= IFF_UP;
		ifr.ifr_flags |= IFF_RUNNING;
		tapcfg_iface_prepare(tapcfg->ifname);
	} else {
		ifr.ifr_flags &= ~IFF_UP;
		ifr.ifr_flags &= ~IFF_RUNNING;
	}

	if (ioctl(tapcfg->ctrl_fd, SIOCSIFFLAGS, &ifr) == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error calling SIOCSIFFLAGS for interface %s: %s\n",
		           tapcfg->ifname,
		           strerror(errno));
		return -1;
	}
	tapcfg->enabled = enabled;

	return 0;
}

int
tapcfg_iface_get_mtu(tapcfg_t *tapcfg)
{
	struct ifreq ifr;
	int ret;

	assert(tapcfg);

	if (!tapcfg->started) {
		return 0;
	}

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, tapcfg->ifname);

	ret = ioctl(tapcfg->ctrl_fd, SIOCGIFMTU, &ifr);
	if (ret == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error getting the MTU of device: %s\n",
		           strerror(errno));
		return -1;
	}

	return ifr.ifr_mtu;
}

int
tapcfg_iface_set_mtu(tapcfg_t *tapcfg, int mtu)
{
	struct ifreq ifr;
	int ret;

	assert(tapcfg);

	if (!tapcfg->started) {
		return 0;
	}

	/* 84 is minimum MTU from RFC 791, we limit the upper
	 * MTU by our internal buffer size minus max header */
	if (mtu < 68 || mtu > (TAPCFG_BUFSIZE - 22)) {
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, tapcfg->ifname);
	ifr.ifr_mtu = mtu;

	ret = ioctl(tapcfg->ctrl_fd, SIOCSIFMTU, &ifr);
	if (ret == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error setting the MTU of device: %s\n",
		           strerror(errno));
		return -1;
	}

	return mtu;
}

int
tapcfg_iface_set_ipv4(tapcfg_t *tapcfg, const char *addrstr, unsigned char netbits)
{
	unsigned int addr, mask;
	int i;

	assert(tapcfg);

	if (!tapcfg->started) {
		return 0;
	}

	if (netbits == 0 || netbits > 32) {
		return -1;
	}

	/* Check that the given IPv4 address is valid */
	if (!tapcfg_address_is_valid(AF_INET, addrstr)) {
		return -1;
	}
	addr = inet_addr(addrstr);

	/* Calculate the netmask from the network bit length */
	for (i=netbits,mask=0; i; i--)
		mask = (mask >> 1)|(1 << 31);

	tapcfg_ifaddr_ioctl(tapcfg->ctrl_fd,
	                    tapcfg->ifname,
	                    addr,
	                    ntohl(mask));

	return 0;
}
