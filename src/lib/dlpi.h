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

/* This file includes the useful DLPI functions we need on Solaris
 * to get and set the hardware address. For more information about
 * DLPI on Solaris, there's a great website available at the address
 * http://www.mm.kay-mueller.de/solaris_drivers.html. Without it
 * writing these functions would've been much more painful */

#include <string.h>
#include <stropts.h>
#include <sys/dlpi.h>

static int
dlpi_put_msg(int fd,
              void *prim, int prim_len,
              void *data, int data_len,
              int flags)
{
	struct strbuf ctrlbuf, databuf;
	int ret;

	ctrlbuf.buf = (char *) prim;
	ctrlbuf.len = prim_len;

	databuf.buf = (char *) data;
	databuf.len = data_len;

	ret = putmsg(fd, &ctrlbuf, (data ? &databuf : NULL), flags);

	return ret;
}

static int
dlpi_get_msg(int fd,
             void *prim, int prim_len,
             void *data, int *data_len,
             int *flags)
{
	struct strbuf ctrlbuf, databuf;
	int tmpflags, ret;

	ctrlbuf.buf = (char *) prim;
	ctrlbuf.maxlen = prim_len;

	databuf.buf = (char *) data;
	databuf.maxlen = (data_len ? *data_len : 0);

	tmpflags = (flags ? *flags : 0);

	ret = getmsg(fd, &ctrlbuf, &databuf, &tmpflags);

	if (data_len)
		*data_len = databuf.len;
	if (flags)
		*flags = tmpflags;

	return ret;
}


#define DLPIBUFSIZE 512


static int
dlpi_attach(int fd, int ppa)
{
	unsigned char buffer[DLPIBUFSIZE];
	dl_attach_req_t dl_attach_req;
	dl_ok_ack_t *p_ok_ack;
	int ret;

	memset(&dl_attach_req, 0, sizeof(dl_attach_req));
	dl_attach_req.dl_primitive = DL_ATTACH_REQ;
	dl_attach_req.dl_ppa = ppa;
	ret = dlpi_put_msg(fd, &dl_attach_req,
	                   sizeof(dl_attach_req),
	                   NULL, 0, 0);
	if (ret < 0) {
		return -1;
	}

	ret = dlpi_get_msg(fd, buffer, sizeof(buffer),
	                   NULL, NULL, 0);
	if (ret != 0) {
		return -1;
	}
	p_ok_ack = (dl_ok_ack_t *) buffer;
	if (p_ok_ack->dl_primitive != DL_OK_ACK) {
		return -1;
	}

	return 0;
}

static int
dlpi_detach(int fd)
{
	unsigned char buffer[DLPIBUFSIZE];
	dl_detach_req_t dl_detach_req;
	dl_ok_ack_t *p_ok_ack;
	int ret;

	memset(&dl_detach_req, 0, sizeof(dl_detach_req));
	dl_detach_req.dl_primitive = DL_DETACH_REQ;
	ret = dlpi_put_msg(fd, &dl_detach_req,
	                   sizeof(dl_detach_req),
	                   NULL, 0, 0);
	if (ret < 0) {
		return -1;
	}

	ret = dlpi_get_msg(fd, buffer, sizeof(buffer),
	                   NULL, NULL, 0);
	if (ret != 0) {
		return -1;
	}
	p_ok_ack = (dl_ok_ack_t *) buffer;
	if (p_ok_ack->dl_primitive != DL_OK_ACK) {
		return -1;
	}

	return 0;
}

static int
dlpi_get_physaddr(int fd, unsigned char *hwaddr, int length)
{
	unsigned char buffer[DLPIBUFSIZE];
	dl_phys_addr_req_t dl_phys_addr_req;
	dl_phys_addr_ack_t *p_phys_addr_ack;
	void *result;
	int ret;

	memset(&dl_phys_addr_req, 0, sizeof(dl_phys_addr_req));
	dl_phys_addr_req.dl_primitive = DL_PHYS_ADDR_REQ;
	dl_phys_addr_req.dl_addr_type = DL_CURR_PHYS_ADDR;
	ret = dlpi_put_msg(fd, &dl_phys_addr_req,
	                   sizeof(dl_phys_addr_req),
	                   NULL, 0, 0);
	if (ret < 0) {
		return -1;
	}

	ret = dlpi_get_msg(fd, buffer, sizeof(buffer),
	                   NULL, NULL, 0);
	if (ret != 0) {
		return -1;
	}
	p_phys_addr_ack = (dl_phys_addr_ack_t *) buffer;
	if (p_phys_addr_ack->dl_primitive != DL_PHYS_ADDR_ACK) {
		return -1;
	}

	if (p_phys_addr_ack->dl_addr_length != length) {
		return -1;
	}

	result = ((char *) p_phys_addr_ack) +
	         p_phys_addr_ack->dl_addr_offset;
	memcpy(hwaddr, result, length);

	return 0;
}

static int
dlpi_set_physaddr(int fd, const char *hwaddr, int length)
{
	unsigned char buffer[DLPIBUFSIZE];
	dl_set_phys_addr_req_t *p_set_phys_addr_req;
	dl_ok_ack_t *p_ok_ack;
	int offset, ret;

	p_set_phys_addr_req = (dl_set_phys_addr_req_t *) buffer;
	memset(p_set_phys_addr_req, 0, sizeof(dl_set_phys_addr_req_t));
	p_set_phys_addr_req->dl_primitive = DL_SET_PHYS_ADDR_REQ;
	p_set_phys_addr_req->dl_addr_length = length;

	offset = sizeof(dl_set_phys_addr_req_t);
	memcpy(buffer + offset, hwaddr, length);
	p_set_phys_addr_req->dl_addr_offset = offset;
	
	ret = dlpi_put_msg(fd, p_set_phys_addr_req,
	                   sizeof(dl_set_phys_addr_req_t) + length,
	                   NULL, 0, 0);
	if (ret < 0) {
		return -1;
	}

	ret = dlpi_get_msg(fd, buffer, sizeof(buffer),
	                   NULL, NULL, 0);
	if (ret != 0) {
		return -1;
	}
	p_ok_ack = (dl_ok_ack_t *) buffer;
	if (p_ok_ack->dl_primitive != DL_OK_ACK) {
		return -1;
	}

	return 0;
}


