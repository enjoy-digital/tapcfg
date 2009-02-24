
#include <sys/dlpi.h>
#include <sys/stropts.h>
#include <string.h>

static int
dlpi_send_msg(int fd,
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
	ret = dlpi_send_msg(fd, &dl_attach_req, sizeof(dl_attach_req), NULL, 0, 0);
	if (ret) {
		return -1;
	}

	dlpi_get_msg(fd, buffer, sizeof(buffer), NULL, NULL, 0);
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
	ret = dlpi_send_msg(fd, &dl_phys_addr_req, sizeof(dl_phys_addr_req), NULL, 0, 0);
	if (ret) {
		return -1;
	}

	dlpi_get_msg(fd, buffer, sizeof(buffer), NULL, NULL, 0);
	p_phys_addr_ack = (dl_phys_addr_ack_t *) buffer;
	if (p_phys_addr_ack->dl_primitive != DL_PHYS_ADDR_ACK) {
		return -1;
	}

	if (p_phys_addr_ack->dl_addr_length != length) {
		return -1;
	}

	result = ((char *) p_phys_addr_ack) + p_phys_addr_ack->dl_addr_offset;
	memcpy(hwaddr, result, length);

	return 0;
}

