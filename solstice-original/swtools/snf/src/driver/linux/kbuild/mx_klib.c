/*************************************************************************
 * The contents of this file are subject to the MYRICOM MYRINET          *
 * EXPRESS (MX) NETWORKING SOFTWARE AND DOCUMENTATION LICENSE (the       *
 * "License"); User may not use this file except in compliance with the  *
 * License.  The full text of the License can found in LICENSE.TXT       *
 *                                                                       *
 * Software distributed under the License is distributed on an "AS IS"   *
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See  *
 * the License for the specific language governing rights and            *
 * limitations under the License.                                        *
 *                                                                       *
 * Copyright 2003 - 2004 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

/* This MX kernel lib code was originally contributed by
 * Brice.Goglin@ens-lyon.org (LIP/INRIA/ENS-Lyon) */

static const char __idstring[] = "@(#)$Id$";

#include "mal_auto_config.h"

#include "mx_arch_klib.h"
#include "mx_arch.h"
#include "mx_instance.h"
#include "mx_malloc.h"
#include "mx_klib.h"

/* all these seem useless for now ? */
int myri_klib_initialized = 0;

mx_klib_mutex_t mx_klib_lock; /* useless ? */
mx_klib_mutex_t Mx_rcache_lock;


#include "snf.h"

int
myri_init_klib(void)
{
  if (!myri_klib_initialized) {
    mx_klib_mutex_init(&mx_klib_lock);
    mx_klib_mutex_init(&Mx_rcache_lock);
    myri_klib_initialized = 1;
  }
  return 0;
}

void
myri_finalize_klib(void)
{
  if (myri_klib_initialized) {
    mx_klib_mutex_destroy(&mx_klib_lock);
    mx_klib_mutex_destroy(&Mx_rcache_lock);
    myri_klib_initialized = 0;
  }
}

int
myri_klib_open(mx_endpt_state_t **es)
{
  *es = NULL;
  return 0;
}

/* 
 * An NULL es means that the external module only open
 * a board but not endpoint.
 * This leads to mx_endptless_ioctl.
 */

int
myri_klib_set_endpoint(mx_endpt_state_t **retes, uint32_t unit,
		       uint32_t endpoint, void *context, int cmd)
{
  enum myri_endpt_type es_type;
  mx_endpt_state_t *es;
  int status;

  es = mx_kmalloc(sizeof(*es), MX_MZERO|MX_WAITOK);
  if (es == 0)
    return ENOMEM;

  es->is_kernel = 1;
  /* set to -1.  Can't set to a real pid, as the process opening
     the endpoint could disappear */
  es->opener.pid = -1;

  switch (cmd) {
    case MX_SET_ENDPOINT:
      es_type = MYRI_ES_MX;
      sprintf(es->opener.comm, "myri_mx_klib");
      break;
    case MX_SET_RAW:
      es_type = MYRI_ES_RAW;
      sprintf(es->opener.comm, "myri_mx_klib_raw");
      es->privileged = 1;
      break;
    case MYRI_SNF_SET_ENDPOINT_RX:
    case MYRI_SNF_SET_ENDPOINT_RX_RING:
    case MYRI_SNF_SET_ENDPOINT_RX_BH:
      if (cmd == MYRI_SNF_SET_ENDPOINT_RX) {
        endpoint = SNF_ENDPOINT_RX;
        sprintf(es->opener.comm, "snf_klib_rx");
      }
      else if (cmd == MYRI_SNF_SET_ENDPOINT_RX_RING) {
        endpoint = SNF_ENDPOINT_RX_RING;
        sprintf(es->opener.comm, "snf_klib_rx_ring");
      }
      else if (cmd == MYRI_SNF_SET_ENDPOINT_RX_BH) {
        endpoint = SNF_ENDPOINT_RX_BH;
        sprintf(es->opener.comm, "snf_kagent");
        es->privileged = 1;
      }
      /* else leave as is */
      es_type = MYRI_ES_SNF_RX;
      break;
    case MYRI_SNF_SET_ENDPOINT_TX:
      sprintf(es->opener.comm, "snf_klib_tx");
      es_type = MYRI_ES_MX;
      break;
    default:
    mx_kfree(es);
    return EINVAL;
    break;
  }

  status = mx_common_open(unit, endpoint, es, es_type);
  if (status != 0) {
    mx_kfree(es);
    return (status);
  }

  myri_update_numa_config(es);

  if (cmd == MX_SET_ENDPOINT) {
    mx_set_endpt_t *set_endpt = (mx_set_endpt_t *)context;
    set_endpt->session_id = es->session_id;
  }
  else if (cmd == MYRI_SNF_SET_ENDPOINT_TX ||
           cmd == MYRI_SNF_SET_ENDPOINT_RX ||
           cmd == MYRI_SNF_SET_ENDPOINT_RX_RING ||
           cmd == MYRI_SNF_SET_ENDPOINT_RX_BH)  {
    status = myri_snf_ioctl(es, cmd, (uaddr_t) context);
    if (status != 0) {
      mx_common_close(es);
      mx_kfree(es);
      return status;
    }
  }
  *retes = es;

  MAL_DEBUG_PRINT (MAL_DEBUG_OPENCLOSE, 
		  ("Board %d, endpoint %d opened\n", 
		   unit, endpoint));
  return 0;
}

int
myri_klib_close(mx_endpt_state_t *es)
{
  if (es != NULL ) {
    mx_common_close(es);
    mx_kfree(es);
  }
  return 0;
}

int
myri_klib_ioctl(mx_endpt_state_t *es, int cmd, void *p, size_t size)
{
  int retval;
  if ( es == NULL ) {
    retval = mx_endptless_ioctl(cmd, (uaddr_t) p, 0, 1);
  } else if (es->es_type == MYRI_ES_SNF_RX) {
    retval = myri_snf_ioctl(es, (uint32_t) cmd, (uaddr_t) p);
  } else {
    retval = mx_common_ioctl(es, (uint32_t) cmd, (uaddr_t) p);
    if (retval == ENOTTY) {
      retval = mx_endptless_ioctl((uint32_t) cmd, (uaddr_t) p, 0, 1);
    }
  }
  return retval;
}

int
myri_klib_map(mx_endpt_state_t *es, uintptr_t offset, void **ptr, mx_page_pin_t **pin)
{
  int mem_type, rc;
  void *kva;
  mx_page_pin_t *pin_req;

  if (pin == NULL)
    pin = &pin_req;

  if (offset == MYRI_OFFSET_UNMAPPED)
    return 0;

  if ((rc = mx_mmap_off_to_kva(es, (unsigned long) offset, &kva,
                               &mem_type, pin)))
    return rc;
  else {
    *ptr = (void *) kva;
    return 0;
  }
}

#if MX_KERNEL_LIB
mx_klib_symbol(snf_init);
mx_klib_symbol(snf_getifaddrs);
mx_klib_symbol(snf_freeifaddrs);

mx_klib_symbol(snf_open);
mx_klib_symbol(snf_open_defaults);
mx_klib_symbol(snf_start);
mx_klib_symbol(snf_stop);
mx_klib_symbol(snf_close);

mx_klib_symbol(snf_ring_open);
mx_klib_symbol(snf_ring_open_id);
mx_klib_symbol(snf_ring_recv);
mx_klib_symbol(snf_ring_getstats);
mx_klib_symbol(snf_ring_close);

mx_klib_symbol(snf_inject_open);
mx_klib_symbol(snf_inject_send);
mx_klib_symbol(snf_inject_getstats);
mx_klib_symbol(snf_inject_close);
#endif /* MX_KERNEL_LIB */
