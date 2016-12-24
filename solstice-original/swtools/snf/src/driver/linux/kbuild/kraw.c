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
 * Copyright 2003 - 2010 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#include "mx_arch.h"
#include "mx_misc.h"
#include "mx_instance.h"
#include "mx_malloc.h"
#include "mx_pio.h"
#include "kraw.h"
#include "myri_raw.h"
#if MYRI_ENABLE_PTP
#include "myri_ptp_common.h"
#endif


void
myri_kraw_intr(mx_instance_state_t *is)
{
  int do_wakeup = 0;

  mx_spin_lock(&is->raw.spinlock);
  if (is->raw.wakeup_needed) {
    do_wakeup = 1;
    is->raw.wakeup_needed = 0;
  }
  mx_spin_unlock(&is->raw.spinlock);

  /* wake up anybody waiting for it */
  if (do_wakeup) {
    mx_wake(&is->raw.sync);
#if MAL_OS_MACOSX
    selwakeup(is->arch.raw_si);
#endif
  }
}

int
myri_kraw_next_event(mx_endpt_state_t *es, myri_raw_next_event_t *e)
{
  mx_instance_state_t *is = es->is;
  mcp_raw_desc_t *mcp_raw_desc;
  myri_raw_tx_buf_t *tx_buf;
  int status = 0;
  uint32_t desc_cnt;
  unsigned long flags;

  flags = 0; /* useless initialization to pacify -Wunused on platforms
                where flags are not used */

  e->status = MYRI_RAW_NO_EVENT;

  desc_cnt = is->raw.desc_cnt;
  mcp_raw_desc = &is->raw.descs[desc_cnt];

  /* see if there is a descriptor pending before sleeping */
  if (mcp_raw_desc->type == MCP_RAW_TYPE_NONE) {
    
    /* if non-blocking, returns immediately */
    if (e->timeout == 0)
      return 0;

    /* tell the interrupt handler to wake us up when
       an event arrives */
    mx_spin_lock_irqsave(&is->raw.spinlock, flags);
    is->raw.wakeup_needed = 1;
    mx_spin_unlock_irqrestore(&is->raw.spinlock, flags);
    mx_sleep(&is->raw.sync, e->timeout, MX_SLEEP_INTR);
    
    /* check to see if the mcp died, since the raw endpoint opener
       will want to know about it */
    if (mx_is_dead(is)) {
      e->status = is->saved_state.reason;
      return 0;
    }

    if (mcp_raw_desc->type == MCP_RAW_TYPE_NONE)
      return 0;
  }

  if (mcp_raw_desc->type == MCP_RAW_TYPE_TX) {
    
    tx_buf = &is->raw.tx_bufs[mcp_raw_desc->tx_id];
    e->status = MYRI_RAW_SEND_COMPLETE;
    e->context = tx_buf->context;
#if MYRI_ENABLE_PTP
    e->timestamp_ns = myri_ptp_tx_nticks_to_nsecs(is, mcp_raw_desc);
#endif
    STAILQ_INSERT_TAIL(&is->raw.tx_bufq, tx_buf, next);
    is->raw.tx_done++;

  } else if (mcp_raw_desc->type == MCP_RAW_TYPE_RX) {
    
    uint32_t rx_cnt, rx_id, page, offset, copyout_len;
    uaddr_t user_buffer;
    void *recv_buffer;
    
    rx_cnt = is->raw.rx_cnt;
    rx_id = rx_cnt & (MCP_RAW_RX_CNT - 1);
    page = rx_id * MCP_RAW_MTU / MX_PAGE_SIZE;
    offset = MX_PAGE_OFFSET(rx_id * MCP_RAW_MTU);
    user_buffer = (uaddr_t)e->recv_buffer;
    recv_buffer = (void *)(uintptr_t)(is->raw.rx_cb.pins[page].va + offset);

    /* adjust copy out length */
    copyout_len = ntohl(mcp_raw_desc->length);
    if (e->recv_bytes < ntohl(copyout_len))
      copyout_len = e->recv_bytes;

    /* copy raw data to user space */
#if MAL_VALGRIND
    e->recv_buffer = (uint64_t)user_buffer;
#endif
    status = mx_copyout(recv_buffer, user_buffer, copyout_len, es->is_kernel);

    e->status = MYRI_RAW_RECV_COMPLETE;
    e->recv_bytes = ntohl(mcp_raw_desc->length);
    e->incoming_port = mcp_raw_desc->port;

    /* consume the receive */
    rx_cnt++;
    is->raw.rx_cnt = rx_cnt;
    MCP_SETVAL(is, raw_rx_cnt, rx_cnt);
    
  } else
    MX_WARN(("Unknown RAW descriptor type (%d)\n", mcp_raw_desc->type));
  
  /* consume the descriptor */
  mcp_raw_desc->type = MCP_RAW_TYPE_NONE;
  is->raw.desc_cnt = (desc_cnt + 1) & ((MX_VPAGE_SIZE / sizeof(mcp_raw_desc_t)) - 1);

  return status;
}

int
myri_kraw_send(mx_endpt_state_t *es, myri_raw_send_t *s)
{
  mx_instance_state_t *is = es->is;
  myri_raw_tx_buf_t *tx_buf;
  mcp_kreq_t kreq;
  mcp_kreq_raw_t *req = &kreq.raw.req;
  int status;
  unsigned long flags;

  flags = 0; /* useless initialization to pacify -Wunused on platforms
                where flags are not used */

  if (s->data_length > MCP_RAW_MTU)
    return EINVAL;

  if (s->physical_port >= is->num_ports)
    return EINVAL;

  if (STAILQ_EMPTY(&is->raw.tx_bufq)) {
#if MYRI_ENABLE_PTP
    MX_WARN(("unable to send PTP message\n"));
#endif
    return EBUSY;
  }

  /* get tx buf */
  tx_buf = STAILQ_FIRST(&is->raw.tx_bufq);
  
  /* copy in the route */
  status = 0;
  if (s->route_length != 0)
    status = mx_copyin((uaddr_t)s->route_pointer, req->route, 
		       sizeof(req->route), es->is_kernel);

  /* copy the raw packet */
  status |= mx_copyin((uaddr_t)s->data_pointer, tx_buf->buf, 
		      s->data_length, es->is_kernel);
  if (status)
    return status;

  /* commit tx buf */
  STAILQ_REMOVE_HEAD(&is->raw.tx_bufq, next);

  /* fill the kernel request */
  req->addr_high = tx_buf->dma.high;   	/* pre-swapped */
  req->addr_low = tx_buf->dma.low;	/* pre-swapped */
  req->data_len = htonl((uint32_t)s->data_length);
  req->route_len = s->route_length;
  req->port = s->physical_port;
  req->tx_id = tx_buf->id;
  req->type = MCP_KREQ_RAW;

  tx_buf->context = s->context;
  is->raw.tx_req++;
  
  mx_spin_lock_irqsave(&is->kreqq_spinlock, flags);
  is->board_ops.write_kreq(is, &kreq);
  mx_spin_unlock_irqrestore(&is->kreqq_spinlock, flags);
  return 0;
}


/*
 * Allocate a big array of vpages, and then stuff them into rx and 
 * tx descriptors.
 */

int
myri_kraw_init(mx_instance_state_t *is)
{
  myri_raw_tx_buf_t *tx_buf;
  int status;
  uint32_t i, page, off, dont_care;

  if (is->raw.tx_bufs != NULL)
    return EBUSY;

  mx_spin_lock_init(&is->raw.spinlock, is, -1, "raw spinlock");  
  mx_sync_init(&is->raw.sync, is, -1, "raw sync");
  STAILQ_INIT(&is->raw.tx_bufq);

  status = mx_mcp_command(is, MCP_CMD_CLEAR_RAW_STATE, 0, 0, 0, &dont_care);
  if (status) {
    MX_WARN(("%s: Could not clear raw state\n", is->is_name));
    mx_spin_lock_destroy(&is->raw.spinlock);
    mx_sync_destroy(&is->raw.sync);
    return ENXIO;
  }

  is->raw.tx_req = 0;
  is->raw.tx_done = 0;
  is->raw.rx_cnt = 0;
  is->raw.desc_cnt = 0;

  /* alloc TX bufs */
  is->raw.tx_bufs = mx_kmalloc(MCP_RAW_TX_CNT * sizeof(is->raw.tx_bufs[0]),
			       MX_WAITOK | MX_MZERO);
  if (is->raw.tx_bufs == NULL) {
    status = ENOMEM;
    goto abort;
  }
  
  /* alloc TX copyblock */
  is->raw.tx_cb.size = MCP_RAW_TX_CNT * MCP_RAW_MTU;
  status = mx_alloc_copyblock(is, &is->raw.tx_cb);
  if (status)
    goto abort;

  /* init TX bufs */
  for (i = 0, tx_buf = is->raw.tx_bufs; i < MCP_RAW_TX_CNT; i++, tx_buf++) {
    page = i * MCP_RAW_MTU / MX_PAGE_SIZE;
    off = MX_PAGE_OFFSET(i * MCP_RAW_MTU);
    tx_buf->dma.high = ntohl(is->raw.tx_cb.pins[page].dma.high);
    tx_buf->dma.low = ntohl(is->raw.tx_cb.pins[page].dma.low + off);
    tx_buf->buf = (void *)(uintptr_t)(is->raw.tx_cb.pins[page].va + off);
    tx_buf->id = i;
    STAILQ_INSERT_TAIL(&is->raw.tx_bufq, tx_buf, next);
  }

  /* alloc RX copyblock */
  is->raw.rx_cb.size = MCP_RAW_RX_CNT * MCP_RAW_MTU;
  status = mx_alloc_copyblock(is, &is->raw.rx_cb);
  if (status)
    goto abort;

  /* init RX ring */
  for (i = 0; i < MCP_RAW_RX_CNT * MCP_RAW_MTU / MX_VPAGE_SIZE; i++) {
    page = i * MX_VPAGE_SIZE / MX_PAGE_SIZE;
    off = MX_PAGE_OFFSET(i * MX_VPAGE_SIZE);
    MCP_SETVAL(is, raw_rx_data[i].high, is->raw.rx_cb.pins[page].dma.high);
    MCP_SETVAL(is, raw_rx_data[i].low, is->raw.rx_cb.pins[page].dma.low + off);
  }

  /* alloc desc ring */
  status = mx_alloc_zeroed_dma_page(is, (void *)&is->raw.descs, &is->raw.descs_pin);
  if (status)
    goto abort;

  /* init desc ring */
  MCP_SETVAL(is, raw_desc.high, is->raw.descs_pin.dma.high);
  MCP_SETVAL(is, raw_desc.low, is->raw.descs_pin.dma.low);
  

  MCP_SETVAL(is, raw_recv_enabled, 1);
  return 0;

 abort:
  myri_kraw_destroy(is);
  return status;
}

void
myri_kraw_tx_wait_pending(mx_instance_state_t *is)
{
  mcp_raw_desc_t *mcp_raw_desc;
  uint32_t desc_cnt, wait_cnt = 0;
  unsigned long flags;

  flags = 0; /* useless initialization to pacify -Wunused on platforms
                where flags are not used */

  desc_cnt = is->raw.desc_cnt;

  while (!mx_is_dead(is) && (is->raw.tx_req != is->raw.tx_done)) {
    
    mcp_raw_desc = &is->raw.descs[desc_cnt];
    if (mcp_raw_desc->type != MCP_RAW_TYPE_NONE) {
      
      /* look for send completion */
      if (mcp_raw_desc->type == MCP_RAW_TYPE_TX)
	is->raw.tx_done++;
      
      /* consume the descriptor */
      mcp_raw_desc->type = MCP_RAW_TYPE_NONE;
      desc_cnt = (desc_cnt + 1) & ((MX_VPAGE_SIZE/sizeof(mcp_raw_desc_t)) - 1);
      is->raw.desc_cnt = desc_cnt;

    } else {
      
      mx_spin_lock_irqsave(&is->raw.spinlock, flags);
      is->raw.wakeup_needed = 1;
      mx_spin_unlock_irqrestore(&is->raw.spinlock, flags);
      mx_sleep(&is->raw.sync, MX_SMALL_WAIT, MX_SLEEP_NOINTR);

      if (!(wait_cnt++))
	MX_WARN(("%s: waiting completion of raw send: %d pending...\n",
		 is->is_name, is->raw.tx_req - is->raw.tx_done));
      if (wait_cnt > 20) {
	MX_WARN(("%s: giving up waiting on raw send completion\n", is->is_name));
	return;
      }
    }
  }
}

void
myri_kraw_destroy(mx_instance_state_t *is)
{
  int status;
  uint32_t dont_care;

  MCP_SETVAL(is, raw_recv_enabled, 0);

  /* Issue a command to clear the raw state.  This is done mainly to
     act as a barrier, so we know we are done processing raw
     interrupts before we start to free resources used in the
     interrupt handler */
  status = mx_mcp_command(is, MCP_CMD_CLEAR_RAW_STATE, 0, 0, 0, &dont_care);
  if (status && !mx_is_dead(is)) {
    MX_WARN(("%s: MCP_CMD_CLEAR_RAW_STATE returns %d in myri_kraw_destroy\n",
	     is->is_name, status));
  }

  if (is->raw.tx_bufs) {
    mx_kfree(is->raw.tx_bufs);
    is->raw.tx_bufs = NULL;
  }
  
  mx_free_copyblock(is, &is->raw.tx_cb);
  mx_free_copyblock(is, &is->raw.rx_cb);

  if (is->raw.descs) {
    mx_free_dma_page(is, (void *)&is->raw.descs, &is->raw.descs_pin);
    is->raw.descs = NULL;
  }

  mx_spin_lock_destroy(&is->raw.spinlock);
  mx_sync_destroy(&is->raw.sync);
}

