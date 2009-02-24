/*
 *  Universal TUN/TAP device driver.
 *
 *  Multithreaded STREAMS tun pseudo device driver. 
 *
 *  Copyright (C) 1999-2000 Maxim Krasnyansky <max_mk@yahoo.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  $Id: tun.c,v 1.12 2000/06/20 03:14:17 maxk Exp $
 */

#include <sys/types.h>
#include <sys/conf.h>
#include <sys/debug.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/strlog.h>
#include <sys/kmem.h>
#include <sys/stat.h>
#include <sys/kstat.h>
#include <sys/modctl.h>
#include <sys/errno.h>
#include <sys/strsun.h>
#include <sys/byteorder.h>

#include <sys/ddi.h>
#include <sys/sunddi.h>

#include <sys/dlpi.h>
#include <sys/ethernet.h>

#include "if_tun.h"

static  int  tunprobe(dev_info_t *);
static	int  tunattach(dev_info_t *, ddi_attach_cmd_t);
static	int  tundetach(dev_info_t *, ddi_detach_cmd_t);
static	int  tuninfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static	int  tunopen(queue_t *, dev_t *, int, int, cred_t *);
static	int  tunclose(queue_t *);
static	int  tunwput(queue_t *wq, mblk_t *mb);
static	int  tunwsrv(queue_t *wq);

static  void tun_frame(queue_t *wq, mblk_t *mpi, int q);

static struct module_info tunminfo = {
  125,		/* mi_idnum  - Module ID number	*/
  "tun",	/* mi_idname - Module name 	*/
  21,		/* mi_minpsz - Min packet size 	*/
  2048,		/* mi_maxpsz - Max packet size 	*/
  (32 * 1024),	/* mi_hiwat  - Hi-water mark 	*/
  21		/* mi_lowat  _ Lo-water mark 	*/
};

static struct qinit tunrinit = {
  NULL, 		/* qi_putp */
  NULL, 		/* qi_srvp */
  tunopen,		/* qi_qopen */
  tunclose,		/* qi_qclose */
  NULL, 		/* qi_qadmin */
  &tunminfo,		/* qi_minfo */
  NULL			/* qi_mstat */
};

static struct qinit tunwinit = {
  tunwput,		/* qi_putp */
  tunwsrv,		/* qi_srvp */
  NULL, 		/* qi_qopen */
  NULL, 		/* qi_qclose */
  NULL, 		/* qi_qadmin */
  &tunminfo,		/* qi_minfo */
  NULL			/* qi_mstat */
};

static struct streamtab	tun_info = {
  &tunrinit,		/* st_rdinit */
  &tunwinit,		/* st_wrinit */
  NULL,			/* st_muxrinit */
  NULL			/* st_muxwrinit */
};

static struct cb_ops tun_cb_ops = {
  nulldev,		/* cb_open */
  nulldev,		/* cb_close */
  nodev,		/* cb_strategy */
  nodev,		/* cb_print */
  nodev,		/* cb_dump */
  nodev,		/* cb_read */
  nodev,		/* cb_write */
  nodev,		/* cb_ioctl */
  nodev,		/* cb_devmap */
  nodev,		/* cb_mmap */
  nodev,		/* cb_segmap */
  nochpoll,		/* cb_chpoll */
  ddi_prop_op,		/* cb_prop_op */
  &tun_info,		/* cb_stream */
  D_NEW | D_MP | D_MTQPAIR | D_MTOUTPERIM | D_MTOCEXCL	/* cb_flag */
};

static	struct dev_ops tun_ops = {
  DEVO_REV,		/* devo_rev */
  0,			/* devo_refcnt */
  tuninfo,		/* devo_getinfo */
  nulldev,		/* devo_identify */
  tunprobe,		/* devo_probe */
  tunattach,		/* devo_attach */
  tundetach,		/* devo_detach */
  nodev,		/* devo_reset */
  &tun_cb_ops,		/* devo_cb_ops */
  NULL,			/* devo_bus_ops */
  ddi_power		/* devo_power */
};

static struct modldrv modldrv = {
  &mod_driverops,	/* Type of module(driver) */
  "TUN/TAP driver "TUN_VER,
  &tun_ops		/* driver ops */
};

static struct modlinkage modlinkage = {
  MODREV_1, { &modldrv, NULL }
};

/* TUN device pointer */
static dev_info_t *tun_dev = NULL;

/* PPA array */
static struct tunppa *tun_ppa[TUNMAXPPA];

/* List of active driver Streams */
static struct tunstr *tun_str;

int _init(void)
{
  cmn_err(CE_CONT, "Universal TUN/TAP device driver ver %s "
		   "(C) 1999-2000 Maxim Krasnyansky\n", TUN_VER);

  DBG(CE_CONT,"tun: _init\n");
  return mod_install(&modlinkage);
}

int _fini(void)
{
  DBG(CE_CONT,"tun: _fini\n");
  return mod_remove(&modlinkage);
}

int _info(struct modinfo *modinfop)
{
  DBG(CE_CONT,"tun: _info\n");
  return mod_info(&modlinkage, modinfop);
}

static int tunprobe(dev_info_t *dev)
{
  DBG(CE_CONT,"tun: tunprobe\n");
  return DDI_PROBE_SUCCESS;
}

static int tunattach(dev_info_t *dev, ddi_attach_cmd_t cmd)
{
  DBG(CE_CONT,"tun: tunattach\n");

  if( cmd == DDI_ATTACH ){
     /* Create the filesystem device node */
     if(ddi_create_minor_node(dev,"tun", S_IFCHR, ddi_get_instance(dev), 
			      DDI_PSEUDO, CLONE_DEV) == DDI_FAILURE) {
	cmn_err(CE_CONT,"tun: ddi_create_minor_node failed");
	return DDI_FAILURE;
     }
     tun_dev = dev;

     ddi_report_dev(dev);
     return (DDI_SUCCESS);
  } else if( cmd == DDI_RESUME ){
     return DDI_SUCCESS;
  } else
     return DDI_FAILURE;
}

static int tundetach(dev_info_t *dev, ddi_detach_cmd_t cmd)
{
  DBG(CE_CONT,"tun: tundetach\n");

  if( cmd == DDI_DETACH ){
     ddi_prop_remove_all(dev);
     ddi_remove_minor_node(dev, NULL);
     return (DDI_SUCCESS);
  } else if( (cmd == DDI_SUSPEND) || (cmd == DDI_PM_SUSPEND) ){
     return (DDI_SUCCESS);
  } else
     return (DDI_FAILURE);
}

/*
 * Translate "dev_t" to a pointer to the associated "dev_info_t".
 */
static int tuninfo(dev_info_t *devi, ddi_info_cmd_t infocmd, void *arg, void **result)
{
  dev_t dev = (dev_t)arg;

  DBG(CE_CONT,"tun: tuninfo\n");

  switch( infocmd ){
     case DDI_INFO_DEVT2DEVINFO:
	*result = tun_dev;
	return DDI_SUCCESS;

     case DDI_INFO_DEVT2INSTANCE:
	*result = (void *)getminor(dev);
	return DDI_SUCCESS;
  }
  return DDI_FAILURE;
}

static int tunopen(queue_t *rq, dev_t *dev, int flag, int sflag, cred_t *credp)
{
  register struct tunstr *str, **prev;
  int minordev, rc = 0;

  /* Determine minor device number */
  prev = &tun_str;
  if( sflag == CLONEOPEN ){
     minordev = 0;
     for(; (str = *prev); prev = &str->s_next){
        if(minordev < str->minor)
           break;
        minordev++;
     }
     *dev = makedevice(getmajor(*dev), minordev);
  } else
     minordev = getminor(*dev);

  if( !rq->q_ptr ){
     str = (struct tunstr *)kmem_zalloc(sizeof(struct tunstr), KM_SLEEP);
     str->rq = rq;
     str->minor = minordev;
     str->state = DL_UNATTACHED;

     str->s_next = *prev;
     *prev = str;

     rq->q_ptr = WR(rq)->q_ptr = (char *)str;
  }
  DBG(CE_CONT,"tun: tunopen str %p minor %d", str, minordev);

  qprocson(rq);
  return rc;
}

static int tunclose(queue_t *rq)
{
  struct tunstr **prev, *tmp, *str=(struct tunstr *)rq->q_ptr;
  struct tunppa *ppa;

  qprocsoff(rq);

  DBG(CE_CONT,"tun: tunclose str %p min %d\n", str, str->minor);

  if( (ppa = str->ppa) ){
     if( str->flags & TUN_CONTROL ){
  	DBG(CE_CONT,"tun: closing control str %p PPA %p\n", str, ppa);
	
	/* Unlink all protocol Streams from the PPA */
	for(tmp = ppa->p_str; tmp; tmp = tmp->p_next){
	   flushq(WR(tmp->rq), FLUSHDATA);
	   tmp->ppa = NULL;
  	   DBG(CE_CONT,"tun: str %p detached from PPA %p\n", tmp, ppa);
	}
       
	/* Free PPA */
	tun_ppa[ppa->id] = NULL;
	kmem_free((char *)ppa, sizeof(struct tunppa));
  	DBG(CE_CONT,"tun: PPA %p removed\n", ppa);
     } else {
	/* Unlink Stream from the PPA list */
	for(prev = &ppa->p_str; (tmp = *prev); prev = &tmp->p_next)
     	   if( tmp==str ) break;
        *prev = tmp->p_next;
  	DBG(CE_CONT,"tun: str %p detached from PPA %p\n", str, ppa);
     }
  }

  /* Unlink Stream from streams list and free it */
  for(prev = &tun_str; (tmp = *prev); prev = &tmp->s_next)
     if( tmp==str ) break;
  *prev = tmp->s_next;
  kmem_free((char *)str, sizeof(struct tunstr));
  rq->q_ptr = WR(rq)->q_ptr = NULL;

  return 0;
}

static void tuniocack(queue_t *wq, mblk_t *mp, int ack, int ret, int err)
{
  struct iocblk *ioc = (struct iocblk *)mp->b_rptr; 

  mp->b_datap->db_type = ack;
  ioc->ioc_count = 0;
  ioc->ioc_error = err;
  ioc->ioc_rval = ret;
  qreply(wq, mp);
}

/* Allocate new PPA */
struct tunppa * tun_alloc_ppa(int id)
{
  struct tunppa * ppa;
  register int i;

  if( id == -1 ){
     /* Find available PPA */
     for(i=0; i < TUNMAXPPA; i++)
	if( !tun_ppa[i] ){
	   id = i; break;
	}
     if( id == -1 )
	return NULL;
  }
  ppa = (struct tunppa *)kmem_zalloc(sizeof(struct tunppa), KM_SLEEP);
  ppa->id = id;
  tun_ppa[id] = ppa;

  return ppa;
}   

/* Handle IOCTLs */
static void tun_ioctl(queue_t *wq, mblk_t *mp)
{
  struct iocblk *ioc = (struct iocblk *)mp->b_rptr; 
  struct tunstr *str = (struct tunstr *)wq->q_ptr; 
  struct tunppa *ppa;
  int p;

  DBG(CE_CONT,"tun: tun_ioctl 0x%x\n", ioc->ioc_cmd);
  switch( ioc->ioc_cmd ){
     case TUNNEWPPA:
	/* Allocate new PPA and assign control stream */

        p = *(int *)mp->b_cont->b_rptr;
	if( p < -1 || p > TUNMAXPPA){
           tuniocack(wq, mp, M_IOCNAK, 0, EINVAL);
	   return;
	}

	if( p != -1 && tun_ppa[p] ){	 
           tuniocack(wq, mp, M_IOCNAK, 0, EEXIST);
	   return;
	}

	if( !(ppa = tun_alloc_ppa(p)) ){
           tuniocack(wq, mp, M_IOCNAK, 0, ENOMEM);
	   return;
	}

	/* Control Stream RQ */	
  	ppa->rq = str->rq;

	str->ppa = ppa;
 	str->flags |= TUN_CONTROL;

        tuniocack(wq, mp, M_IOCACK, ppa->id, 0);

  	DBG(CE_CONT,"tun: new PPA %d control str %p\n", ppa->id, str);
        break;

     case TUNSETPPA:
	/* Attach stream to a PPA */
        p = *(int *)mp->b_cont->b_rptr;
	if( p < 0 || p > TUNMAXPPA){
           tuniocack(wq, mp, M_IOCNAK, 0, EINVAL);
	   return;
	}

	if( !(ppa = tun_ppa[p]) ){	 
           tuniocack(wq, mp, M_IOCNAK, 0, ENODEV);
	   break;
	}
	str->p_next = ppa->p_str;
	ppa->p_str  = str;
	
	str->ppa = ppa;

        tuniocack(wq, mp, M_IOCACK, p, 0);

  	DBG(CE_CONT,"tun: str %p attached to PPA %d \n", str, p);
        break;

     case DLIOCRAW:          /* Raw M_DATA mode */
        str->flags |= TUN_RAW;
        tuniocack(wq, mp, M_IOCACK, 0, 0);
        break;

     default:
        tuniocack(wq, mp, M_IOCNAK, 0, EINVAL);
        break;
  }
}

static void tunerr(queue_t *wq, int error)
{
  mblk_t *mp;

  if( !(mp = allocb(1, BPRI_LO)) )
     return;
  mp->b_datap->db_type = M_ERROR;
  mp->b_rptr = mp->b_datap->db_base;
  mp->b_wptr = mp->b_rptr + 1;
  *(int32_t *)mp->b_rptr = -1;
  *mp->b_rptr = (uint8_t)error;

  qreply(wq, mp);
}

static mblk_t *tunchmsg(mblk_t *mp, int size, uint8_t type, int32_t prim)
{
  if( !mp || (MBLKSIZE(mp) < size) || (DB_REF(mp) > 1) ){
     if( mp ) 
	freemsg(mp);
     if( !(mp = allocb(size, BPRI_LO)) )
        return NULL;
  }
     
  mp->b_datap->db_type = type;
  mp->b_rptr = mp->b_datap->db_base;
  mp->b_wptr = mp->b_rptr + size;
  if(prim >= 0)
     *(int32_t *)mp->b_rptr = prim;
  return mp;
}

static void tundlokack(queue_t *wq, mblk_t *mp, uint32_t prim)
{
  union DL_primitives *dlp;

  if( !(mp = tunchmsg(mp, sizeof(dl_ok_ack_t), M_PCPROTO, DL_OK_ACK)) ){
     tunerr(wq, ENOSR);
     return;
  }
  dlp = (union DL_primitives *)mp->b_rptr;
  dlp->ok_ack.dl_correct_primitive = prim;
  qreply(wq, mp);
}

static void tundlerrack(queue_t *wq, mblk_t *mp, uint32_t  errprim,
			uint32_t errno, uint32_t uerrno)
{
  union DL_primitives *dlp;

  if( !(mp=tunchmsg(mp, sizeof(dl_error_ack_t), M_PCPROTO, DL_ERROR_ACK)) ){
     tunerr(wq, ENOSR);
     return;
  }
  dlp = (union DL_primitives *)mp->b_rptr;
  dlp->error_ack.dl_error_primitive = errprim;
  dlp->error_ack.dl_errno = errno;
  dlp->error_ack.dl_unix_errno = uerrno;
  qreply(wq, mp);
}

static dl_info_ack_t tun_dl_info = {
  DL_INFO_ACK,                    /* dl_primitive */
  ETHERMTU,                       /* dl_max_sdu */
  0,                              /* dl_min_sdu */
  TUN_ADDR_LEN,                   /* dl_addr_length */
  DL_ETHER,                       /* dl_mac_type */
  0,                              /* dl_reserved */
  0,                              /* dl_current_state */
  -2,                             /* dl_sap_length */
  DL_CLDLS,                       /* dl_service_mode */
  0,                              /* dl_qos_length */
  0,                              /* dl_qos_offset */
  0,                              /* dl_range_length */
  0,                              /* dl_range_offset */
  DL_STYLE2,                      /* dl_provider_style */
  sizeof(dl_info_ack_t),          /* dl_addr_offset */
  DL_VERSION_2,                   /* dl_version */
  0,		                  /* dl_brdcst_addr_length */
  0,				  /* dl_brdcst_addr_offset */
  0                               /* dl_growth */
};

static void tun_info_req(queue_t *wq, mblk_t *mp)
{
  struct tunstr *str = (struct tunstr *)wq->q_ptr;
  struct tundladdr *dla;
  dl_info_ack_t *dli;
  int size;

  if(MBLKL(mp) < DL_INFO_REQ_SIZE){
     tundlerrack(wq, mp, DL_INFO_REQ, DL_BADPRIM, 0);
     return;
  }

  DBG(CE_CONT,"tun: tun_info_req str %p\n", str);

  size = sizeof(dl_info_ack_t) + TUN_ADDR_LEN;
  if( !(mp = tunchmsg(mp, size, M_PCPROTO, DL_INFO_ACK)) ){
     tunerr(wq, ENOSR);
     return;
  }

  /* Fill DL_INFO_ACK struct */
  dli = (dl_info_ack_t *)mp->b_rptr;
  bcopy(&tun_dl_info, dli, sizeof(dl_info_ack_t));
  dli->dl_current_state = str->state;
  dla = (struct tundladdr *)(mp->b_rptr + dli->dl_addr_offset);
  dla->sap = str->sap;

  qreply(wq, mp);
}

static void tun_attach_req(queue_t *wq, mblk_t *mp)
{
  union DL_primitives *dlp = (union DL_primitives *)mp->b_rptr;
  struct tunstr *str = (struct tunstr *)wq->q_ptr;
  struct tunppa *ppa;
  int p;

  DBG(CE_CONT,"tun: tun_attach_req %p\n", str);

  if(MBLKL(mp) < DL_ATTACH_REQ_SIZE) {
     tundlerrack(wq, mp, DL_ATTACH_REQ, DL_BADPRIM, 0);
     return;
  }

  if(str->state != DL_UNATTACHED) {
     tundlerrack(wq, mp, DL_ATTACH_REQ, DL_OUTSTATE, 0);
     return;
  }

  p = dlp->attach_req.dl_ppa;

  /* Attach stream to a PPA */
  if( p < 0 || p > TUNMAXPPA){
     tundlerrack(wq, mp, dlp->dl_primitive, DL_BADPPA, 0);
     return;
  }

  if( !(ppa = tun_ppa[p]) ){	 
     tundlerrack(wq, mp, dlp->dl_primitive, DL_BADPPA, 0);
     return;
  }
  str->p_next = ppa->p_str;
  ppa->p_str = str;

  str->ppa = ppa;
  str->state = DL_UNBOUND;

  tundlokack(wq, mp, DL_ATTACH_REQ);

  DBG(CE_CONT,"tun: str %p attached to PPA %d \n", str, p);
}

static void tun_detach_req(queue_t *wq, mblk_t *mp)
{
  struct tunstr *tmp, **prev, *str = (struct tunstr *)wq->q_ptr;
  struct tunppa *ppa = str->ppa;

  DBG(CE_CONT,"tun: tun_detach_req %p\n", str);

  if(MBLKL(mp) < DL_DETACH_REQ_SIZE){
     tundlerrack(wq, mp, DL_DETACH_REQ, DL_BADPRIM, 0);
     return;
  }

  if(str->state != DL_UNBOUND){
     tundlerrack(wq, mp, DL_DETACH_REQ, DL_OUTSTATE, 0);
     return;
  }

  /* Unlink from PPA list */
  for(prev = &ppa->p_str; (tmp = *prev); prev = &tmp->p_next)
     if( tmp == str ) break;
  *prev = tmp->p_next;
  DBG(CE_CONT,"tun: str %p detached from PPA %p\n", str, ppa);

  str->ppa = NULL;
  str->state = DL_UNATTACHED;

  tundlokack(wq, mp, DL_DETACH_REQ);
}

static void tun_bind_req(queue_t *wq, mblk_t *mp)
{
  static struct tundladdr dladdr;
  union  DL_primitives  *dlp;
  struct tunstr *str;
  int size, xidtest;
  uint32_t sap;

  str = (struct tunstr *)wq->q_ptr;
  dlp = (union DL_primitives *)mp->b_rptr;

  if(MBLKL(mp) < DL_BIND_REQ_SIZE) {
     tundlerrack(wq, mp, DL_BIND_REQ, DL_BADPRIM, 0);
     return;
  }

  if(str->state != DL_UNBOUND) {
     tundlerrack(wq, mp, DL_BIND_REQ, DL_OUTSTATE, 0);
     return;
  }

  sap = dlp->bind_req.dl_sap;
  xidtest = dlp->bind_req.dl_xidtest_flg;

  DBG(CE_CONT,"tun: tun_bind_req str %p sap %d\n", str, sap);

  if(xidtest){
     tundlerrack(wq, mp, DL_BIND_REQ, DL_NOAUTO, 0);
     return;
  }
  if(sap > ETHERTYPE_MAX) {
     tundlerrack(wq, mp, dlp->dl_primitive, DL_BADSAP, 0);
     return;
  }

  str->sap = sap;
  str->state = DL_IDLE;

  DBG(CE_CONT,"tun: str %p bound to sap %d\n", str, sap);

  dladdr.sap = sap;

  size = sizeof(dl_bind_ack_t) + TUN_ADDR_LEN;
  if( !(mp = tunchmsg(mp, size, M_PCPROTO, DL_BIND_ACK)) ){
     tunerr(wq, ENOSR);
     return;
  }
  dlp = (union DL_primitives *)mp->b_rptr;
  dlp->bind_ack.dl_sap = sap;
  dlp->bind_ack.dl_addr_length = TUN_ADDR_LEN;
  dlp->bind_ack.dl_addr_offset = sizeof (dl_bind_ack_t);
  dlp->bind_ack.dl_max_conind = 0;
  dlp->bind_ack.dl_xidtest_flg = 0;
  bcopy((caddr_t)&dladdr, (caddr_t)(mp->b_rptr + sizeof(dl_bind_ack_t)), TUN_ADDR_LEN);
  qreply(wq, mp);
}

static void tun_unbind_req(queue_t *wq, mblk_t *mp)
{
  struct tunstr *str = (struct tunstr *)wq->q_ptr;

  DBG(CE_CONT,"tun: tun_unbind_req str %p\n", str);

  if(MBLKL(mp) < DL_UNBIND_REQ_SIZE){
     tundlerrack(wq, mp, DL_UNBIND_REQ, DL_BADPRIM, 0);
     return;
  }

  if(str->state != DL_IDLE) {
     tundlerrack(wq, mp, DL_UNBIND_REQ, DL_OUTSTATE, 0);
     return;
  }

  str->state = DL_UNBOUND;
  str->sap = 0;

  tundlokack(wq, mp, DL_UNBIND_REQ);
}

static void tun_promiscon_req(queue_t *wq, mblk_t *mp)
{
  struct tunstr *str = (struct tunstr *)wq->q_ptr;

  DBG(CE_CONT,"tun: tun_promiscon_req str %p\n", str);

  if( MBLKL(mp) < DL_PROMISCON_REQ_SIZE ){
     tundlerrack(wq, mp, DL_PROMISCON_REQ, DL_BADPRIM, 0);
     return;
  }

  switch( ((dl_promiscon_req_t *)mp->b_rptr)->dl_level ){
     case DL_PROMISC_PHYS:
        DBG(CE_CONT,"tun: PROMISC_PHYS str %p\n", str);
        str->flags |= TUN_ALL_PHY;
        break;

     case DL_PROMISC_SAP:
        DBG(CE_CONT,"tun: PROMISC_SAP str %p\n", str);
        str->flags |= TUN_ALL_SAP;
        break;

     case DL_PROMISC_MULTI:
        DBG(CE_CONT,"tun: PROMISC_MULTI str %p\n", str);
        str->flags |= TUN_ALL_MUL;
        break;

     default:
        tundlerrack(wq, mp, DL_PROMISCON_REQ, DL_NOTSUPPORTED, 0);
        return;
  }
  tundlokack(wq, mp, DL_PROMISCON_REQ);
}

static void tun_promiscoff_req(queue_t *wq, mblk_t *mp)
{
  struct tunstr *str = (struct tunstr *)wq->q_ptr;

  DBG(CE_CONT,"tun: tun_promiscoff_req str %p\n", str);

  if(MBLKL(mp) < DL_PROMISCOFF_REQ_SIZE){
     tundlerrack(wq, mp, DL_PROMISCOFF_REQ, DL_BADPRIM, 0);
     return;
  }

  switch( ((dl_promiscoff_req_t *)mp->b_rptr)->dl_level ){
     case DL_PROMISC_PHYS:
        str->flags &= ~TUN_ALL_PHY;
        break;

     case DL_PROMISC_SAP:
        str->flags &= ~TUN_ALL_SAP;
        break;

     case DL_PROMISC_MULTI:
        str->flags &= ~TUN_ALL_MUL;
        break;

     default:
        tundlerrack(wq, mp, DL_PROMISCOFF_REQ, DL_NOTSUPPORTED, 0);
        return;
  }
  tundlokack(wq, mp, DL_PROMISCOFF_REQ);
}

void tun_physaddr_req(queue_t *wq, mblk_t *mp)
{
  union   DL_primitives   *dlp;
  struct  ether_addr addr;
  int size;
#ifdef TUN_DEBUG
  struct tunstr *str = (struct tunstr *)wq->q_ptr;

  DBG(CE_CONT,"tun: tun_physaddr_req str %p\n", str);
#endif

  if( MBLKL(mp) < DL_PHYS_ADDR_REQ_SIZE ){
     tundlerrack(wq, mp, DL_PHYS_ADDR_REQ, DL_BADPRIM, 0);
     return;
  }
  bzero(&addr, sizeof(addr));
  size = sizeof(dl_phys_addr_ack_t) + ETHERADDRL;
  if( !(mp = tunchmsg(mp, size, M_PCPROTO, DL_PHYS_ADDR_ACK)) )
     return;
  dlp = (union DL_primitives *)mp->b_rptr;
  dlp->physaddr_ack.dl_addr_length = ETHERADDRL;
  dlp->physaddr_ack.dl_addr_offset = sizeof(dl_phys_addr_ack_t);
  bcopy(&addr, (caddr_t)(mp->b_rptr + sizeof(dl_phys_addr_ack_t)), ETHERADDRL);
  qreply(wq, mp);
}

static void tun_unitdata_req(queue_t *wq, mblk_t *mp)
{
  struct tunstr *str = (struct tunstr *)wq->q_ptr;
  struct tunppa *ppa = str->ppa; 
  mblk_t *nmp;

  DBG(CE_CONT,"tun: tun_unitdata_req str %p data %d\n", str, msgdsize(mp));

  if(str->state != DL_IDLE || !ppa ){
     tundlerrack(wq, mp, DL_UNITDATA_REQ, DL_OUTSTATE, 0);
     return;
  }

  if( !(nmp = mp->b_cont) ){
     tundlerrack(wq, mp, DL_UNITDATA_REQ, DL_BADDATA, 0);
     return;
  }

  /* Drop unidata_req part of the message */
  mp->b_cont = NULL;
  freemsg(mp);  

  /* Route frame */
  tun_frame(wq, nmp, TUN_QUEUE); 
}

static mblk_t * tun_unitdata_ind(mblk_t *mp, int type)
{
  dl_unitdata_ind_t *ud_ind;
  struct tundladdr *dla;
  mblk_t *nmp;
  int size;

  DBG(CE_CONT,"tun: tun_unitdata_ind \n");

  /* Allocate new mblk */
  size = sizeof(dl_unitdata_ind_t) + TUN_ADDR_LEN + TUN_ADDR_LEN;
  if( !(nmp = allocb(size, BPRI_LO)) ){
     freemsg(mp);
     return NULL;
  }
  DB_TYPE(nmp) = (uint8_t)M_PROTO;
  nmp->b_wptr = nmp->b_datap->db_lim;
  nmp->b_rptr = nmp->b_wptr - size;

  /* Construct DL_UNITDATA_IND message */
  ud_ind = (dl_unitdata_ind_t *)nmp->b_rptr;
  ud_ind->dl_primitive = DL_UNITDATA_IND;
  ud_ind->dl_dest_addr_length = TUN_ADDR_LEN;
  ud_ind->dl_dest_addr_offset = sizeof(dl_unitdata_ind_t);
  ud_ind->dl_src_addr_length = TUN_ADDR_LEN;
  ud_ind->dl_src_addr_offset = sizeof(dl_unitdata_ind_t) + TUN_ADDR_LEN;
  ud_ind->dl_group_address = 0;

  dla = (struct tundladdr *)(nmp->b_rptr + ud_ind->dl_dest_addr_offset);
  dla->sap = (uint16_t)type;
  dla = (struct tundladdr *)(nmp->b_rptr + ud_ind->dl_src_addr_offset);
  dla->sap = (uint16_t)type;

  nmp->b_cont = mp;
  return nmp;
}

static mblk_t * tun_eth_hdr(mblk_t *mp, int type)
{
  mblk_t *nmp;
  int size;

  DBG(CE_CONT,"tun: tun_eht_hdr \n");

  /* Allocate new mblk */
  size = sizeof(struct ether_header);
  if( !(nmp = allocb(size, BPRI_LO)) ){
     freemsg(mp);
     return NULL;
  }
  DB_TYPE(nmp) = (uint8_t)M_DATA;
  nmp->b_wptr += sizeof(struct ether_header);
  bzero(nmp->b_rptr, sizeof(struct ether_header));
  ((struct ether_header *)nmp->b_rptr)->ether_type = htons(type);
  nmp->b_cont = mp;
  return nmp;
}


/* Route frames */
static void tun_frame(queue_t *wq, mblk_t *mp, int q)
{
  struct tunstr *str = (struct tunstr *)wq->q_ptr;
  register struct tunppa *ppa;
  register struct tunstr *tmp;
  mblk_t *nmp;

  if( !(ppa = str->ppa) ){
     /* Stream is not attached to PPA. Ignore frame. */
     DBG(CE_CONT,"tun: unattached str %p, dropping frame\n", str);

     freemsg(mp);
     return; 
  }

  DBG(CE_CONT,"tun: tun_frame str %p PPA %d\n", str, ppa->id);

  /* Check for the sniffers */
  for( tmp=ppa->p_str; tmp; tmp = tmp->p_next ){
     if( SNIFFER(tmp->flags) && canputnext(tmp->rq) ){
	if( !(nmp = dupmsg(mp)) )
	   continue;

  	DBG(CE_CONT,"tun: frame %d -> sniffer %p\n", msgdsize(nmp), tmp);

        if( tmp->flags & TUN_RAW ){
           if( (nmp=tun_eth_hdr(nmp, ETHERTYPE_IP)) ) 
	      putnext(tmp->rq, nmp);
	   continue;
	}          
        if( (nmp=tun_unitdata_ind(nmp, ETHERTYPE_IP)) )
           putnext(tmp->rq, nmp);
     }
  }

  if( !(str->flags & TUN_CONTROL) ){
     /* Data from the Protocol stream send it to 
      * the Control stream */ 
     DBG(CE_CONT,"tun: frame %d -> control str\n", msgdsize(mp));
     if( canputnext(ppa->rq) ){
	putnext(ppa->rq, mp);
     } else {
	if( q == TUN_QUEUE ){
           DBG(CE_CONT,"tun: queueing frame %d\n", msgdsize(mp));
	   putbq(wq, mp);
	} else {
           DBG(CE_CONT,"tun: dropping frame %d\n", msgdsize(mp));
           freemsg(mp);
	}
     }
  } else {
     /* Data from the Control stream.  
      * Route frame to the Protocol streams. */
     for( tmp=ppa->p_str; tmp; tmp = tmp->p_next ){
        if( tmp->sap==ETHERTYPE_IP &&  canputnext(tmp->rq) ){
	   if( !(nmp = dupmsg(mp)) )
	      continue;

  	   DBG(CE_CONT,"tun: frame %d -> proto %p\n", msgdsize(nmp), tmp);

           if( tmp->flags & TUN_RAW ){
              if( (nmp=tun_eth_hdr(nmp, ETHERTYPE_IP)) ) 
	         putnext(tmp->rq, nmp);
	      continue;
	   }          
           if( (nmp=tun_unitdata_ind(nmp, ETHERTYPE_IP)) )
              putnext(tmp->rq, nmp);
	}
     }
     /* Free original message */
     freemsg(mp);
  }
}

static void tun_dlpi(queue_t *wq, mblk_t *mp)
{
  union DL_primitives *dlp = (union DL_primitives *)mp->b_rptr;
  uint32_t prim = dlp->dl_primitive;

  switch( prim ){
     case DL_INFO_REQ:
        tun_info_req(wq, mp);
        break;

     case DL_ATTACH_REQ:
	qwriter(wq, mp, tun_attach_req, PERIM_OUTER);
        break;

     case DL_DETACH_REQ:
	qwriter(wq, mp, tun_detach_req, PERIM_OUTER);
        break;

     case DL_BIND_REQ:
        tun_bind_req(wq, mp);
        break;

     case DL_UNBIND_REQ:
        tun_unbind_req(wq, mp);
        break;

     case DL_PROMISCON_REQ:
	tun_promiscon_req(wq, mp);
	break;

     case DL_PROMISCOFF_REQ:
	tun_promiscoff_req(wq, mp);
	break;

     case DL_PHYS_ADDR_REQ:
	tun_physaddr_req(wq, mp);
	break;

     case DL_ENABMULTI_REQ:
     case DL_DISABMULTI_REQ:
     default:
        tundlerrack(wq, mp, prim, DL_UNSUPPORTED, 0);
        break;
  }
}

static int tunwput(queue_t *wq, mblk_t *mp)
{
  union DL_primitives *dlp;
  uint32_t prim;
#ifdef TUN_DEBUG
  struct tunstr *str = (struct tunstr *)wq->q_ptr;

  DBG(CE_CONT, "tun: tunwput str %p\n", str);
#endif

  switch( DB_TYPE(mp) ){
     case M_DATA:
	tun_frame(wq, mp, TUN_QUEUE);
	break;

     case M_PROTO:
     case M_PCPROTO:
        dlp = (union DL_primitives *)mp->b_rptr;
        prim = dlp->dl_primitive;

        switch( prim ){
	   case DL_UNITDATA_REQ:
              tun_unitdata_req(wq, mp);
              break;
	   default:
	      /* Queue other DLPI messages for wsrv */
	      putq(wq, mp);
	      break;
	}
	break;

     case M_IOCTL:
	qwriter(wq, mp, tun_ioctl, PERIM_OUTER);
	break;

     case M_FLUSH:
	/* Flush queues */
        if(*mp->b_rptr & FLUSHW) {
           flushq(wq, FLUSHALL);
           *mp->b_rptr &= ~FLUSHW;
        }
        if(*mp->b_rptr & FLUSHR)
           qreply(wq, mp);
        else
           freemsg(mp);

	break;

     default:
        freemsg(mp);
        break;
  }
  return 0;
}

static int tunwsrv(queue_t *wq)
{
  mblk_t *mp;
#ifdef TUN_DEBUG
  struct tunstr *str = (struct tunstr *)wq->q_ptr;

  DBG(CE_CONT,"tun: tunwsrv str %p\n", str);
#endif

  while( (mp = getq(wq)) )
     switch( DB_TYPE(mp) ){
 	 case M_DATA:
	    tun_frame(wq, mp, TUN_DROP);
	    break;

	 case M_PROTO:
	 case M_PCPROTO:
	    tun_dlpi(wq, mp);
   	    break;

	 default:
	    freemsg(mp);
	    break;
     }
  return 0;
}
