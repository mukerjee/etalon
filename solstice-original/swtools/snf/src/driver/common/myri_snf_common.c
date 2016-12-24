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
#include "mx_instance.h"
#include "mx_klib.h"

#if MYRI_SNF_KAGENT
#include "snf_rx.h"
#include "snf_rx_event.h"
static int myri_snf_rx_kagent_start(myri_snf_rx_state_t *snf, const myri_snf_rx_params_t *x);
static int myri_snf_rx_kagent_stop(myri_snf_rx_state_t *snf);
#endif

#if MAL_OS_LINUX
#define MYRI_SNF_RX_KMALLOC 1
#else
#define MYRI_SNF_RX_KMALLOC 0
#endif

uint32_t myri_snf_rings = 0;
uint32_t myri_snf_flags = -1;

void myri_dump_snf_state(myri_snf_rx_state_t *snf);
int myri_ether_soft_rx(mx_instance_state_t *is, myri_soft_rx_t *r);

void 
myri_dump_snf_state(myri_snf_rx_state_t *snf)
{
  int i, nrings;
  MX_WARN(("attach_mask=%#x, refcount=%d, state=%d, rx_s=%p\n",
        snf->attach_mask, snf->refcount, snf->state, snf->rx_s));
  MX_WARN(("desc_cb [kva=%p, size=%lu, pins=%p, pinned_pages=%lu\n",
        (void *) snf->desc_cb.kva, (unsigned long) snf->desc_cb.size,
        snf->desc_cb.pins, snf->desc_cb.pinned_pages));
  MX_WARN(("data_cb [kva=%p, size=%lu, pins=%p, pinned_pages=%lu\n",
        (void *) snf->data_cb.kva, (unsigned long) snf->data_cb.size,
        snf->data_cb.pins, snf->data_cb.pinned_pages));
  nrings = snf->params.num_rings;
  if (!nrings)
    nrings = MYRI_SNF_MAX_RINGS;
  for (i = 0; i < nrings; i++) {
    MX_WARN(("ring.%2d [kva=%p, hcb = {kva=%p, size=%lu, pins=%p, pinned_pages=%lu}]\n ",
          i, (void *) snf->rx_ring_cbs[i].kva, (void *) snf->rx_ring_cbs[i].hcb.kva, 
          (unsigned long) snf->rx_ring_cbs[i].hcb.size, snf->rx_ring_cbs[i].hcb.pins, 
          snf->rx_ring_cbs[i].hcb.pinned_pages));
  }
  MX_WARN(("rings_attached=%d\n", snf->num_rings_attached));
}


int
myri_poll_events_pending(mx_endpt_state_t *es)
{
  int revents = 0;
  uint8_t *tx_done_vec;

  if (es->flow_window && es->es_type == MYRI_ES_MX) {
    tx_done_vec = (uint8_t *)(uintptr_t) es->is->tx_done.pin.va;
    if ((uint8_t) es->flow_window->tx_flow.flow_desc != tx_done_vec[es->endpt])
      revents |= MYRI_WAIT_TX_EVENT;
  }
  return revents;
}

uint32_t
myri_snf_intr_req(mx_endpt_state_t *es, uint32_t intr_flags)
{
  myri_snf_rx_state_t *snf = &es->is->snf;
  unsigned long flags;
  uint32_t intr_flags_set = 0;

  flags = 0; /* -Wunused on non-linux */

  /* Ensure only one interrupt request is in flight */
  if (intr_flags & MYRI_WAIT_RX_EVENT) {
    mx_spin_lock_irqsave(&snf->rx_intr_lock, flags);
    if (snf->rx_intr_requested == 0 && snf->state == MYRI_SNF_RX_S_STARTED) {
      MX_PIO_WRITE((uint32_t *)(es->is->lanai.sram + 0x7c0000),
                   htonl(snf->rx_s->evq.evq_flow_desc));
      intr_flags_set |= MYRI_WAIT_RX_EVENT;
      snf->rx_intr_requested = 1;
      snf->rx_intr_es = es;
    }
    mx_spin_unlock_irqrestore(&snf->rx_intr_lock, flags);
  }
  if (intr_flags & MYRI_WAIT_TX_EVENT && es->flow_window) {
    mal_assert(es->endpt < myri_max_endpoints); /* real mcp endpoints only */
    mx_spin_lock_irqsave(&es->snf.tx_lock, flags);
    if (es->snf.tx_intr_requested == 0) {
      MX_PIO_WRITE((uint32_t *)(es->is->lanai.sram + 0x7c0004 + (es->endpt << 4)),
                   htonl(es->flow_window->tx_flow.flow_desc));
      es->snf.tx_intr_requested = 1;
      intr_flags_set |= MYRI_WAIT_TX_EVENT;
    }
    mx_spin_unlock_irqrestore(&es->snf.tx_lock, flags);
  }
  if (intr_flags_set) /* Requested at least one interrupt, flush doorbell write */
    MAL_STBAR();
  return intr_flags_set;
}

int
myri_snf_intr_tx_try_flush(mx_endpt_state_t *es)
{
  struct snf__tx_state *ss = es->snf.tx_sp.tx_s;

  if (es->flags & MX_ES_CLOSING) /* stop trying to flush */
    return 1;

  if (!snf__spin_trylock(&ss->tx_lock))
    return 0;

  snf__tx_update_completion(&es->snf.tx_sp);
  if (ss->txd.d_cnt && ss->dq_inflight == 0) {
    snf__tx_pioflush(&es->snf.tx_sp);
    ss->tx_flush_intr_cnt++;
  }
  snf__spin_unlock(&ss->tx_lock);
  return 1;
}

void
myri_poll_prep(mx_endpt_state_t *es)
{
  /* Only TX interrupt for now */
  myri_snf_intr_req(es, MYRI_WAIT_TX_EVENT);
}

int
myri_poll_wait(mx_endpt_state_t *es, int timeout_ms, int events)
{
  int status;

  if (timeout_ms < 0)
    timeout_ms = MX_MAX_WAIT;

  if (timeout_ms == 0) {
    /* We're not going to block below in sleep, but claim the interrupt if it
     * is available.  This is mostly useful when myri_poll_wait is called with
     * timeout_ms==0 when the endpoint is used with poll() */
    status = mx_sleep(&es->wait_sync, 0, MX_SLEEP_INTR);
    if (status != EAGAIN) {
      MX_WARN(("unexpected return from myri_poll_wait: %d\n", status));
      return status;
    }
  }
  else {
    if (events) {
      myri_snf_intr_req(es, events);
    }
    status = mx_sleep(&es->wait_sync, timeout_ms, MX_SLEEP_INTR);

    if (status == EINTR)
      return status;
    else if (status != 0 && status != EAGAIN) {
      MX_WARN(("unexpected return from myri_poll_wait: %d\n", status));
      return status;
    }
  }

  return status;
}

int
myri_snf_mmap_off_to_kva(mx_endpt_state_t *es, unsigned long req, 
                         unsigned long *off, void **kva,
		         int *mem_type, mx_page_pin_t **pin)
{
  void *tmpkva;
  mx_instance_state_t *is = es->is;
  int index, i;
  myri_snf_rx_state_t *snf = &is->snf;

  /* mapping snf 0: snf rx doorbell page */
  if (is->snf.rx_doorbell_be64 && is->snf.rx_s) {
    tmpkva = (void *)((char *)is->snf.rx_doorbell_be64 + (req - *off));
    *off += MX_PAGE_SIZE;
    if (req < *off) {
      *kva = tmpkva;
      *mem_type = MX_MEM_SRAM;
      return 0;
    }
  }

  /* mapping snf 1: snf rx state */
  if (is->snf.rx_s) {
    tmpkva = (void *)((char *)is->snf.rx_s + (req - *off));
    *off += is->snf.rx_s_length;
    if (req < *off) {
      *kva = tmpkva;
      *mem_type = MX_MEM_HOSTMEM;
      return 0;
    }
  }

  /* mapping snf 2: snf desc addrs */
  if (is->snf.desc_cb.size != 0) {
    index = (req - *off) / PAGE_SIZE;
    *off += is->snf.desc_cb.size;
    if (req < *off) {
      *mem_type = MX_MEM_HOSTMEM;
      if (is->snf.desc_cb.kva != NULL)
        *kva = (void *)((uintptr_t)is->snf.desc_cb.kva + PAGE_SIZE * index);
      else
	*kva = (void *)(uintptr_t)is->snf.desc_cb.pins[index].va;
      *pin = &is->snf.desc_cb.pins[index];
      return 0;
    }
  }

  /* mapping snf 3: snf data addrs */
  if (is->snf.data_cb.size != 0) {
    index = (req - *off) / PAGE_SIZE;
    *off += is->snf.data_cb.size;
    if (req < *off) {
      *mem_type = MX_MEM_HOSTMEM;
      if (is->snf.data_cb.kva != NULL)
        *kva = (void *)((uintptr_t)is->snf.data_cb.kva + PAGE_SIZE * index);
      else
	*kva = (void *)(uintptr_t)is->snf.data_cb.pins[index].va;
      *pin = &is->snf.data_cb.pins[index];
      return 0;
    }
  }

  /* mapping snf 4: software ring addresses */
  for (i = 0; i < snf->params.num_rings; i++) {
    index = (req - *off) / PAGE_SIZE;
    *off += snf->rx_ring_hcb_size;
    if (req < *off) {
      if (!(snf->attach_mask & (1<<i)))
        continue;
      if (snf->rx_ring_cbs[i].kva) {
        *kva = (void *)((uintptr_t)snf->rx_ring_cbs[i].kva + PAGE_SIZE * index);
        *pin = NULL;
      }
      else {
        if (snf->rx_ring_cbs[i].hcb.kva != NULL)
          *kva = (void *)((uintptr_t)snf->rx_ring_cbs[i].hcb.kva + PAGE_SIZE * index);
        else
          *kva = (void *)(uintptr_t)snf->rx_ring_cbs[i].hcb.pins[index].va;
        *pin = &snf->rx_ring_cbs[i].hcb.pins[index];
      }
      *mem_type = MX_MEM_HOSTMEM;
      return 0;
    }
  }

  return ENOENT;
}

#if MYRI_SNF_KAGENT
void
myri_snf_kagent_body(myri_snf_rx_state_t *snf)
{
  if (snf->kagent.state == MYRI_SNF_KAGENT_STARTED)
    myri_snf_recv_agent(snf->kagent.rx, 10);
}
#endif

static int
myri_snf_start_stop(mx_endpt_state_t *es, int start)
{
  mx_instance_state_t *is = es->is;
  uint32_t dummy;
  int status;
  status = mx_mcp_command(is, MCP_CMD_SNF_RECV, start, 0, 0, &dummy);
  return status;
}

static
int
myri_snf_rx_detach(mx_endpt_state_t *es)
{
  myri_snf_rx_state_t *snf = &es->is->snf;
  int i, ring_id = es->snf.ring_id;
  unsigned long flags;

  flags = 0;  /* useless initialization to pacify -Wunused */

  if (es->snf.es_type != MYRI_SNF_ES_RX_RING)
    return EINVAL;

  mal_assert(ring_id >= 0);

  if (snf->attach_mask & (1<<ring_id)) {
    snf->attach_mask &= ~(1<<ring_id);
    es->is->kernel_window->attach_mask = snf->attach_mask;
    snf->rx_s->pq.reclaim_mask &= ~(1<<ring_id);
    /* Prevent further enqueues */
    snf->rx_s->vrings[ring_id].vr_p.seq = 0;
    snf->rx_s->vrings[ring_id].vr_c.seq = 1;
    if (snf->params.open_flags & (MYRI_SNF_F_RX_DUP|MYRI_SNF_F_RX_COPY)) {
      snf->rx_s->vrings[ring_id].vr_c.dataq_off = 0;
      snf->rx_s->vrings[ring_id].vr_p.dataq_off = 0;
    }
    else {
      snf->rx_s->vrings[ring_id].vr_c.dataq_off = -1;
      snf->rx_s->vrings[ring_id].vr_p.dataq_off = -1;
    }
    snf->num_rings_attached--;
    snf->refcount--;
    snf->ring_map_ep[ring_id] = NULL;

    if (snf->kagent.state == MYRI_SNF_KAGENT_DISABLED) {
      mx_spin_lock_irqsave(&snf->rx_intr_lock, flags);
      if (snf->rx_intr_requested == 1 && snf->rx_intr_es == es) {
        /* If an interrupt was requested and we requested it, pass the request
         * along to the next ring, if any */
        ring_id = snf__ffs(snf->attach_mask);
        if (ring_id)
          snf->rx_intr_es = snf->ring_map_ep[ring_id-1];
        else
          snf->rx_intr_es = NULL;
      }
      mx_spin_unlock_irqrestore(&snf->rx_intr_lock, flags);
      /* in case we owned the pq, make it free again. */
      if (ring_id == snf__cmpswap_u32(&snf->rx_s->pq.ring_id,
                                      ring_id, SNF_RING_UNUSED)) {
        uint32_t mask = snf->attach_mask;
        /* We owned the PQ, wake up everyone else and make them fight for PQ */
        while ((i = snf__ffs(mask)) > 0) {
          i--;
          mask &= ~(1<<i);
          if (i != ring_id && snf->ring_map_ep[i])
            mx_wake(&snf->ring_map_ep[i]->wait_sync);
        }
      }
    }
  }
  es->snf.ring_id = SNF_RING_UNUSED;
  return 0;
}

#if MAL_OS_LINUX
static int
myri_snf_tx_open(mx_endpt_state_t *es, myri_snf_tx_params_t *x)
{
  unsigned long host_offset;

  host_offset = 0;
  x->tx_data_ring_size = es->sendq.size;
  x->tx_data_ring_offset = host_offset; /* mapping 1 */
  host_offset += es->sendq.size;
  host_offset += es->recvq.size; /* mapping 2 */
  host_offset += es->eventq.size; /* mapping 3 */

  x->tx_desc_ring_size = es->user_mmapped_sram.size;
  x->tx_desc_ring_offset = (uint64_t) host_offset; /* mapping 4 */
  host_offset += es->user_mmapped_sram.size;

  x->tx_doorbell_size = es->user_mmapped_zreq.size;
  x->tx_doorbell_offset = (uint64_t) host_offset; /* mapping 5 */
  host_offset += es->user_mmapped_zreq.size;
  host_offset += es->is->kernel_window ? MX_PAGE_SIZE : 0; /* mapping 6 */

  x->tx_flow_offset = host_offset; /* mapping 7: per-ep flow page */
  x->tx_flow_size = MX_PAGE_SIZE;
  host_offset += MX_PAGE_SIZE;

  x->tx_compl_offset = (uint64_t) host_offset; /* mapping 8 */
  x->tx_compl_size = MX_PAGE_SIZE;
  host_offset += MX_PAGE_SIZE;

  es->snf.es_type = MYRI_SNF_ES_TX;

  es->snf.tx_sp.txd_cnt = MX_VPAGE_SIZE / (SNF_TX_DESC_PKT_MAX * 2);
  es->snf.tx_sp.txd_dbaddr_be32 = (uint32_t *) es->user_mmapped_zreq.addr;
  es->snf.tx_sp.txd_pio_base = (uintptr_t) es->user_mmapped_sram.addr;
  es->snf.tx_sp.tx_s = (struct snf__tx_state *) es->flow_window;
  es->snf.tx_sp.txd_compl_update = (uint8_t *) es->is->tx_done.addr + es->endpt;

  sprintf(es->opener.user_info, "tx ring %d", es->endpt);
  return 0;
}
#endif

static void
myri_snf_rx_get_kva_maps(const mx_endpt_state_t *es, myri_snf_rx_attach_t *x)
{
  unsigned long host_offset;
  const myri_snf_rx_state_t *snf = &es->is->snf;

  host_offset = 0;

  /*
   * figure out the offset the same way mmap will
   * so that the lib knows what offset to pass to
   * mmap to get the rings
   */
  host_offset = 0;
  host_offset += es->sendq.size;
  host_offset += es->recvq.size; /* mapping 2 */
  host_offset += es->eventq.size; /* mapping 3 */
  host_offset += es->user_mmapped_sram.size; /* mapping 4 */
  host_offset += es->user_mmapped_zreq.size; /* mapping 5 */

  x->kernel_window_offset = (uint64_t) host_offset; /* mapping 6 */
  x->kernel_window_size = MX_PAGE_SIZE;
  host_offset += x->kernel_window_size;

  host_offset += es->flow_window ? MX_PAGE_SIZE : 0; /* mapping 7 */
  host_offset += es->is->tx_done.addr ? MX_PAGE_SIZE : 0; /* mapping 8 */

  x->rx_doorbell_offset = (uint64_t) host_offset; /* mapping snf 0 */
  x->rx_doorbell_size = MX_PAGE_SIZE;
  host_offset += MX_PAGE_SIZE;

  x->vstate_offset = (uint64_t) host_offset; /* mapping snf 1 */
  x->vstate_size = snf->rx_s_length;
  host_offset += snf->rx_s_length;

  x->rx_desc_ring_offset = (uint64_t) host_offset; /* mapping snf 2 */
  host_offset += snf->desc_cb.size;

  x->rx_data_ring_offset = (uint64_t) host_offset;
  host_offset += snf->data_cb.size;
}


static int
myri_snf_rx_attach(mx_endpt_state_t *es, myri_snf_rx_attach_t *x)
{
  myri_snf_rx_state_t *snf = &es->is->snf;
  int ring_id = x->ring_id;
  int rc = 0;
  int node_id = -1;
  unsigned long host_offset;
  unsigned long flags;
  struct myri_snf_rx_ring_p *rx_p;

  flags = 0;  /* useless initialization to pacify -Wunused */

  if (es->snf.es_type != MYRI_SNF_ES_FREE)
    return EINVAL;

  mx_mutex_enter(&snf->sync);
  if (ring_id != -1 && ring_id >= snf->params.num_rings)
    rc = EINVAL;
  else if (snf->num_rings_attached == snf->params.num_rings ||
           (ring_id >= 0 && (snf->attach_mask & (1<<ring_id)))) {
    MX_WARN(("EBUSY in rx_attach: ring_id=%d, attached=%d, nrings=%d\n",
            ring_id, snf->num_rings_attached, snf->params.num_rings));
    rc = EBUSY;
  }
  else {
    if (ring_id < 0) {
      /* Find next free ring */
      for (ring_id = 0; ring_id < snf->params.num_rings; ring_id++) {
        if (!(snf->attach_mask & (1<<ring_id)))
          break;
      }
      if (ring_id == snf->params.num_rings) {
        rc = EBUSY;
        goto bail;
      }
    }

    myri_snf_rx_get_kva_maps(es, x);

    host_offset = x->rx_data_ring_offset + snf->data_cb.size;
    rx_p = &snf->rx_ring_cbs[ring_id];

    node_id = myri_snf_find_node_id(es);

#if MYRI_SNF_RX_KMALLOC
    /* Allocate copyblocks for desc and data, only if required.  Rings may
     * attach/detach while some others are still active.  The rings are only
     * deallocated when sniffer is completely unloaded */
    if (rx_p->kva == NULL) {
      rx_p->kva = mx_kmalloc_node(snf->rx_ring_hcb_size, MX_WAITOK|MX_MZERO, node_id);
      if (rx_p->kva == NULL)
        rc = ENOMEM;
#else
    /* Use huge copyblock allocator instead */
    if (rx_p->hcb.size == 0) {
      rx_p->hcb.size = snf->rx_ring_hcb_size;
      rc = mx_alloc_huge_copyblock(es->is, &rx_p->hcb, 0, node_id);
#if MAL_OS_LINUX
      /* We only need to map the copyblock if the kernel agent is in use or
       * if this is a kernel endpoint */
      if (rc == 0 && (es->is_kernel || (snf->kagent.state != MYRI_SNF_KAGENT_DISABLED))) {
        if (!mx_map_huge_copyblock(&rx_p->hcb)) {
          mx_free_huge_copyblock(es->is, &rx_p->hcb);
          MX_WARN(("Unable to kmap ring %d copyblock\n", ring_id));
          rc = ENXIO;
        }
      }
#endif
#endif /* MYRI_SNF_RX_KMALLOC */
      if (rc) {
        MX_WARN(("Failed to allocate snf ring %d, es=%p\n", ring_id, es));
        rc = ENOMEM;
        goto bail;
      }
      rx_p->p.ring_id = ring_id;
      rx_p->p.rx_desc_ring_count = snf->rx_ring_desc_count;
      rx_p->p.rx_desc_ring_size = snf->rx_ring_desc_size;
      rx_p->p.rx_data_ring_size = snf->rx_ring_data_size;

      if (snf->rx_ring_desc_size)
        rx_p->p.rx_desc_ring_offset =
          host_offset + snf->rx_ring_hcb_size * ring_id;
      else
        rx_p->p.rx_desc_ring_offset = MYRI_OFFSET_UNMAPPED;

      if (snf->rx_ring_data_size) {
        rx_p->p.rx_data_ring_preamble_size = snf->params.rx_data_preamble_size;
        rx_p->p.rx_data_ring_offset = 
          rx_p->p.rx_desc_ring_offset + rx_p->p.rx_desc_ring_size;
      }
      else {
        rx_p->p.rx_data_ring_preamble_size = 0;
        rx_p->p.rx_data_ring_offset = MYRI_OFFSET_UNMAPPED;
      }
    }

    if (!(snf->params.rx_map_flags & MYRI_SNF_RXMAP_PQ_DESC))
      x->rx_desc_ring_offset = MYRI_OFFSET_UNMAPPED;
    if (!(snf->params.rx_map_flags & MYRI_SNF_RXMAP_PQ_DATA))
      x->rx_data_ring_offset = MYRI_OFFSET_UNMAPPED;

    x->params = snf->params;
    x->ring_id = ring_id;
    snf->rx_s->vrings[ring_id].vr_p.seq = 0;
    snf->rx_s->vrings[ring_id].vr_c.seq = 0;
    snf->attach_mask |= (1<<ring_id);
    es->is->kernel_window->attach_mask = snf->attach_mask;
    snf->num_rings_attached++;
    snf->refcount++;
    snf->ring_map_ep[ring_id] = es;
    snf->rx_ring_node_id = node_id;
    es->snf.ring_id = ring_id;
    es->snf.es_type = MYRI_SNF_ES_RX_RING;

    mx_spin_lock_irqsave(&es->is->clk_spinlock, flags);
    x->nticks = es->is->clk_nticks;
    mx_spin_unlock_irqrestore(&es->is->clk_spinlock, flags);
    sprintf(es->opener.user_info, "rx ring %d", ring_id);
  }

bail:
  mx_mutex_exit(&snf->sync);
  return rc;
}

void
myri_snf_rx_teardown(mx_endpt_state_t *es)
{
  myri_snf_rx_state_t *snf = &es->is->snf;
  mx_instance_state_t *is = es->is;
  uint32_t dummy;
  int i, status;
  int ring_id;

  if (es->es_type != MYRI_ES_SNF_RX  || /* must be a Sniffer RX type */
      es->snf.es_type == MYRI_SNF_ES_FREE) /* No Sniffer resources allocated yet */
    return;

  ring_id = es->snf.ring_id;

  mx_mutex_enter(&snf->sync);
  if (es->snf.es_type == MYRI_SNF_ES_RX_RING)
    myri_snf_rx_detach(es);
  else
    snf->refcount--;

#if MYRI_SNF_KAGENT
  /* See if we have to teardown the kagent started implicitly  */
  if (snf->kagent.state != MYRI_SNF_KAGENT_DISABLED &&
      snf->refcount == 1 && snf->kagent.es != es)
  {
    if (snf->state == MYRI_SNF_RX_S_STARTED)
      myri_snf_start_stop(es, 0);
    snf->state = MYRI_SNF_RX_S_STOPPED;
    mal_assert(snf->kagent.es != NULL);

    snf->kagent.state = MYRI_SNF_KAGENT_STOPPING;
    mx_mutex_exit(&snf->sync);

    /* kagent stop will close the thread's endpoint which will call back into
     * this function and decrement refcount to 0 */
    myri_snf_rx_kagent_stop(snf);
    es->snf.es_type = MYRI_SNF_ES_FREE;
    return;
  }
#endif

  if (snf->kagent.es == es) {
    snf->kagent.state = MYRI_SNF_KAGENT_DISABLED;
    snf->kagent.es = NULL;
    if (snf->kagent.rx) {
      mx_kfree(snf->kagent.rx);
      snf->kagent.rx = NULL;
    }
  }

  if (snf->refcount == 0) {
    if (snf->state == MYRI_SNF_RX_S_STARTED)
      myri_snf_start_stop(es, 0);
    snf->state = MYRI_SNF_RX_S_STOPPED;

    status = mx_mcp_command(is, MCP_CMD_SNF_RING, 0, 0, 0, &dummy);
    if (status) {
      MX_WARN (("could not unmap snf mcp ring: %d\n", status));
    }

    if (snf->rx_s != NULL) {
      mx_kfree(snf->rx_s);
      snf->rx_s = NULL;
      snf->rx_s_length = 0;
    }

    mx_free_huge_copyblock(is, &snf->data_cb);
    mx_free_huge_copyblock(is, &snf->desc_cb);

    for (i = 0; i < snf->params.num_rings; i++) {
      if (snf->rx_ring_cbs[i].kva != NULL) { /* used kmalloc */
        mx_kfree(snf->rx_ring_cbs[i].kva);
        snf->rx_ring_cbs[i].kva = NULL;
      }
      mx_free_huge_copyblock(is, &snf->rx_ring_cbs[i].hcb);
    }
    snf->num_rings_attached = 0;
    snf->params.num_rings = 0;
    snf->state = MYRI_SNF_RX_S_FREE;
    snf->rx_ring_hcb_size = 0;
    snf->owner_pid = -1;
  }
  mx_mutex_exit(&snf->sync);
  es->snf.es_type = MYRI_SNF_ES_FREE;
}

static
void
myri_snf_setup_rx_state(myri_snf_rx_state_t *snf)
{
  struct snf__rx_state *rx_s = snf->rx_s;
  int i;

  bzero(rx_s, sizeof(struct snf__rx_state));

  /* pq */
  rx_s->pq.reclaim_mask = 0;
  rx_s->pq.ring_id = SNF_RING_UNUSED;

  /* Eventq */
  rx_s->evq.evq_seqnum = 1;
  rx_s->evq.evq_last_idx = (uint32_t)
    (snf->params.rx_desc_ring_size / sizeof(const mcp_snf_rx_desc_t) - 1);

  /* Dataq */
  rx_s->recvq.rq_ev_ts_last = 1;
  rx_s->recvq.rq_data_size = snf->params.rx_data_ring_size;

  /* Flow state */
  rx_s->flow_rx.flow_desc = 
    (uint32_t) (snf->params.rx_desc_ring_size >> SNF_VPAGE_SHIFT);
  rx_s->flow_rx.flow_data = 
    (uint32_t) (snf->params.rx_data_ring_size >> SNF_VPAGE_SHIFT);

  for (i = 0; i < MYRI_SNF_MAX_RINGS; i++) {
    rx_s->vrings[i].vr_p.seq = 0;
    rx_s->vrings[i].vr_c.seq = 1;
    rx_s->vrings[i].vr_p.pkt_drops = 0;
    rx_s->vrings[i].vr_p.pkt_drops = 0;
    rx_s->vrings[i].vr_p.pkt_drops_data = 0;
    rx_s->vrings[i].vr_p.pkt_recvd = 0;
    if (snf->params.open_flags & (MYRI_SNF_F_RX_DUP|MYRI_SNF_F_RX_COPY)) {
      rx_s->vrings[i].vr_c.dataq_off = 0;
      rx_s->vrings[i].vr_c.dataq_off = 0;
    }
    else {
      rx_s->vrings[i].vr_c.dataq_off = -1;
      rx_s->vrings[i].vr_p.dataq_off = -1;
    }
  }
}

static int
myri_snf_kmap_copyblocks(mx_endpt_state_t *es)
{
  if (!es->is_kernel)
    return 0;

#if MAL_OS_LINUX
  /* For kernel-based endpoints, make sure that the copyblocks are virtually
   * mapped */
  if (es->is_kernel) {
    myri_snf_rx_state_t *snf = &es->is->snf;
    if (snf->desc_cb.kva == NULL) {
      if (!mx_map_huge_copyblock(&snf->desc_cb)) {
        MX_WARN(("Failed to kmap snf desc copyblock\n"));
        return ENXIO;
      }
    }
    if (snf->data_cb.kva == NULL) {
      if (!mx_map_huge_copyblock(&snf->data_cb)) {
        MX_WARN(("Failed to kmap snf data copyblock, es=%p\n", es));
        return ENXIO;
      }
    }
  }
#endif
  return 0;
}

/* If kagent, valid modes are
 *   BORROW, COPY, COPY_DUP
 */

static int
myri_snf_rx_open_get_params(mx_endpt_state_t *es, myri_snf_rx_params_t *x)
{
  mx_instance_state_t *is = es->is;
  myri_snf_rx_state_t *snf = &is->snf;
  unsigned long meta_size;
  int kagent = 0;

  if (es->snf.ring_id == SNF_RING_KAGENT && !es->is_kernel)
    return EACCES;

  if (snf->state != MYRI_SNF_RX_S_FREE) {
    if (es->snf.ring_id == SNF_RING_KAGENT) {
      if (snf->kagent.state != MYRI_SNF_KAGENT_DISABLED) {
        MX_WARN(("SNF kagent already in use!\n"));
        return EBUSY;
      }
    }
    else if (!es->is_kernel && mx_kgetpid() != snf->owner_pid && 
             !(snf->params.open_flags & 0x1)) {
      MX_WARN(("SNF can't share rings between processes unless SNF_F_PSHARED "
               "mode is used (owner is PID %d, request from PID %d)\n", 
               snf->owner_pid, mx_kgetpid()));
      return EACCES;
    }
    *x = snf->params;
    return 0;
  }

  if (myri_snf_rings != 0)
    x->num_rings = myri_snf_rings;

  if (myri_snf_flags != -1)
    x->open_flags = myri_snf_flags;

#if MAL_OS_LINUX
  if (x->open_flags & MYRI_SNF_F_RX_KAGENT_ENABLE) 
    kagent = 1;
  else if (x->open_flags & MYRI_SNF_F_RX_KAGENT_DISABLE)
    kagent = 0;
  else
    kagent = !!(MYRI_SNF_F_RX_KAGENT_DEFAULT & MYRI_SNF_F_RX_KAGENT_ENABLE);
#endif

  /* If there's only one ring, ignore all dup/copy requests */
  if (x->num_rings == 1)
    x->open_flags &= ~(MYRI_SNF_F_RX_COPY|MYRI_SNF_F_RX_DUP);
  if (kagent)
    x->open_flags |= MYRI_SNF_F_RX_KAGENT_ENABLE;

  x->rx_map_flags = 0;
  if (!kagent) {
    x->rx_map_flags |= MYRI_SNF_RXMAP_PQ_DESC | MYRI_SNF_RXMAP_PQ_DATA;
    if (x->num_rings > 1) {
      x->rx_map_flags |= MYRI_SNF_RXMAP_VQ_DESC;
      if ((x->open_flags & (MYRI_SNF_F_RX_COPY|MYRI_SNF_F_RX_DUP)))
        x->rx_map_flags |= MYRI_SNF_RXMAP_VQ_DATA;
    }
  }
  else {
    x->rx_map_flags |= MYRI_SNF_RXMAP_VQ_DESC;
    if (x->open_flags & (MYRI_SNF_F_RX_COPY|MYRI_SNF_F_RX_DUP))
      x->rx_map_flags |= MYRI_SNF_RXMAP_VQ_DATA;
    else
      x->rx_map_flags |= MYRI_SNF_RXMAP_PQ_DATA;
  }

  if (kagent && !es->is_kernel && (x->rss_flags & SNF_RSS_FLAGS_CUSTOM_FUNC)) {
    MX_WARN(("%s: SNF can't use custom RSS in userspace with kagent\n",
          es->is->is_name));
    return EINVAL;
  }

  /* Validate that sizes are a multiple of 2 MB */
  meta_size = (unsigned long)
    (MX_VPAGE_SIZE * MX_VPAGE_SIZE / sizeof(mcp_dma_addr_t));

  if ((unsigned long)x->rx_data_ring_size & (meta_size - 1) ||
      (unsigned long)x->rx_desc_ring_size & (meta_size - 1)) {
    MX_WARN(("Desc or Data ring size not a multiple of 2MB\n"));
    return EINVAL;
  }
  if (x->num_rings > MYRI_SNF_MAX_RINGS) {
    MX_WARN(("Unsupported number of rings: %d\n", x->num_rings));
    return EINVAL;
  }

  x->rx_data_preamble_size = SNF_ALIGNUP(SNF_MTUWRAP_SIZE, PAGE_SIZE);

  return 0;
}

static int
myri_snf_rx_join(mx_endpt_state_t *es)
{
  int status = 0;

  if (es->is_kernel && ((status = myri_snf_kmap_copyblocks(es)) != 0))
    return status;

  return status;
}

static int
myri_snf_rx_enable(mx_endpt_state_t *es, myri_snf_rx_params_t *x)
{
  mx_instance_state_t *is = es->is;
  myri_snf_rx_state_t *snf = &is->snf;
  uint32_t offset;
  unsigned long flags;
  int status = 0;
  unsigned long meta_size;
  int node_id;

  flags = 0;  /* useless initialization to pacify -Wunused */

  meta_size = (unsigned long)
    (MX_VPAGE_SIZE * MX_VPAGE_SIZE / sizeof(mcp_dma_addr_t));
  /* allocate rings in NIC */
  status = mx_mcp_command(is, MCP_CMD_SNF_RING, 1, 
	        x->rx_data_ring_size / (meta_size / sizeof(mcp_dma_addr_t)), 
		x->rx_desc_ring_size / (meta_size / sizeof(mcp_dma_addr_t)), 
		&offset);
  if (status) {
    MX_WARN (("could not get snf ring offset\n"));
    return status;
  }

  /* Get a reference count now in case allocation fails */
  
  snf->rx_intr_es = NULL;
  snf->rx_intr_requested = 0;

  /* In the preamble, we stuff enough for data premables */
  snf->data_cb.size = x->rx_data_ring_size + x->rx_data_preamble_size;
  
  node_id = myri_snf_find_node_id(es);

  /* allocate the huge copyblocks */
  status = mx_alloc_huge_copyblock(is, &snf->data_cb, 
                                   x->rx_data_preamble_size, node_id);
  if (status) {
    MX_WARN(("Failed to allocate snf data copyblock\n"));
    return ENOMEM;
  }
  status = mx_dma_map_copyblock(es, &snf->data_cb.cb, offset);
  if (status) {
    MX_WARN(("Failed to map snf data copyblock\n"));
    return ENXIO;
  }

  snf->desc_cb.size = x->rx_desc_ring_size;
  status = mx_alloc_huge_copyblock(is, &snf->desc_cb, 0, node_id);
  if (status) {
    MX_WARN(("Failed to allocate snf desc copyblock\n"));
    return ENOMEM;
  }
  offset += x->rx_data_ring_size >> (2*(MX_VPAGE_SHIFT-3));
  status = mx_dma_map_copyblock(es, &snf->desc_cb.cb, offset);
  if (status) {
    MX_WARN(("Failed to map snf desc copyblock\n"));
    return ENXIO;
  }

  snf->rx_s_length = SNF_ALIGNUP(sizeof(struct snf__rx_state), PAGE_SIZE);
  snf->rx_s = mx_kmalloc(snf->rx_s_length, MX_WAITOK|MX_MZERO);
  if (snf->rx_s == NULL) {
    MX_WARN(("Failed to alloc rx state page\n"));
    return ENOMEM;
  }

  if (es->is_kernel && ((status = myri_snf_kmap_copyblocks(es)) != 0))
    return status;

  snf->rx_ring_hcb_size = 0;
  if (x->rx_map_flags & MYRI_SNF_RXMAP_VQ_DESC)
    snf->rx_ring_desc_size = snf->desc_cb.size;
  else 
    snf->rx_ring_desc_size = 0;
  snf->rx_ring_hcb_size += snf->rx_ring_desc_size;
  snf->rx_ring_desc_count = snf->rx_ring_desc_size / SNF_VTOKEN_SIZE;

  if (x->rx_map_flags & MYRI_SNF_RXMAP_VQ_DATA) {
    snf->rx_ring_data_size = x->rx_data_ring_size;
    snf->rx_ring_hcb_size += snf->rx_ring_data_size + x->rx_data_preamble_size;
  }
  else 
    snf->rx_ring_data_size = 0;

  myri_snf_clear_stats(is, 1 /* is reset */);

  snf->owner_pid = mx_kgettgpid();
  snf->params = *x; /* Save params, these are the default now */
  snf->state = MYRI_SNF_RX_S_INIT;

  myri_snf_setup_rx_state(snf);
  snf__rx_flow_update((uint64_t *) snf->rx_doorbell_be64, &snf->rx_s->flow_rx);

  return 0;
}

static int
myri_snf_rx_open(mx_endpt_state_t *es, myri_snf_rx_params_t *x)
{
  mx_instance_state_t *is = es->is;
  myri_snf_rx_state_t *snf = &is->snf;
  int status = 0;

  mx_mutex_enter(&snf->sync);
  /* First do all parameter checking where failures won't consume any SNF
   * resources */
  if ((status = myri_snf_rx_open_get_params(es, x))) {
    mx_mutex_exit(&snf->sync);
    return status;
  }

  snf->refcount++;
  es->snf.es_type = MYRI_SNF_ES_RX;
  if (es->snf.ring_id == SNF_RING_KAGENT) {
    snf->kagent.state = MYRI_SNF_KAGENT_STARTING;
    snf->kagent.es = es;
  }

  if (snf->state == MYRI_SNF_RX_S_FREE) { /* First Sniffer10G opener */
    if ((status = myri_snf_rx_enable(es, x)))
      goto fail;

#if MYRI_SNF_KAGENT
    if ((snf->params.open_flags & MYRI_SNF_F_RX_KAGENT_ENABLE) && 
        es->snf.ring_id != SNF_RING_KAGENT) {
      /* Ensure nobody tries to own the pqueue */
      snf->rx_s->pq.ring_id = SNF_RING_KAGENT;
      mx_mutex_exit(&snf->sync);
      
      /* Start the agent with its own SNF RX endpoint */
      status = myri_snf_rx_kagent_start(snf, x);
      mx_mutex_enter(&snf->sync);
      if (status)
        goto fail;
    }
#endif
  } else {
    if ((status = myri_snf_rx_join(es)))
      goto fail;
  }
  sprintf(es->opener.user_info, "rx handle (%d %s rings)", 
      snf->params.num_rings, 
      (snf->params.open_flags & 0x1) ? "shared" : "private");

 fail:
  mx_mutex_exit(&snf->sync);
  if (status)
    myri_snf_rx_teardown(es);
  return status;
}

static
void
myri_snf_update_stats(mx_instance_state_t *is, myri_snf_stats_t *stats)
{
  stats->snf_pkt_recv = myri_get_counter(is, MCP_COUNTER_SNF_RECV);
  stats->snf_pkt_overflow = myri_get_counter(is, MCP_COUNTER_SNF_OVERFLOW);
  stats->nic_pkt_overflow = myri_get_counter(is, MCP_COUNTER_NIC_OVERFLOW);
  stats->nic_pkt_bad = myri_get_counter(is, MCP_COUNTER_BAD_PKT);
  stats->nic_bytes_recv = myri_get_counter(is, MCP_COUNTER_NIC_RECV_KBYTES);
  stats->nic_pkt_send = myri_get_counter(is, MCP_COUNTER_SNF_SEND);
  stats->nic_bytes_send = myri_get_counter(is, MCP_COUNTER_NIC_SEND_KBYTES);
  return;
}

void
myri_snf_get_stats(mx_instance_state_t *is, myri_snf_stats_t *stats_o)
{
  uint64_t delta, *st_prev, *st_accum, *st_now, *st_get;
  myri_snf_stats_t stats_now;
  int i;

  st_prev  = (uint64_t *) &is->snf_stats_prev;
  st_accum = (uint64_t *) &is->snf_stats;
  st_now   = (uint64_t *) &stats_now;
  st_get   = (uint64_t *) stats_o;

  myri_snf_update_stats(is, &stats_now);

  for (i = 0; i < (int) sizeof(myri_snf_stats_t)/sizeof(uint64_t); i++) {
    delta = (st_now[i] - st_prev[i]) & 0xffffffffULL;
    if (i == offsetof(myri_snf_stats_t, nic_bytes_recv)/sizeof(uint64_t) ||
        i == offsetof(myri_snf_stats_t, nic_bytes_send)/sizeof(uint64_t))
      delta *= 1024;
    st_accum[i] += delta;
    st_get[i] = st_accum[i];
    st_prev[i] = st_now[i];
  }
}

void
myri_snf_clear_stats(mx_instance_state_t *is, int reset)
{
  /* If resetting, we want the accumulated values to be zero */
  if (reset)
    bzero(&is->snf_stats, sizeof(myri_snf_stats_t));

  myri_snf_update_stats(is, &is->snf_stats_prev);
}

int
myri_snf_ioctl(mx_endpt_state_t *es, uint32_t cmd, const uaddr_t in)
{
  uaddr_t out = in;
  int status = ENOTTY;
  myri_snf_rx_state_t *snf = &es->is->snf;

  if (mx_is_dead(es->is)) {
    /* if the board is dead, return an error to the application...
     * except in case of parity since the recovery is notified to the
     * application through the wait_status or get_board_status
     * (other ioctls can still be processed safely since they do not
     * touch the board for anything that won't be redone a afterwards)
     */
    if (!(es->is->flags & MX_PARITY_RECOVERY)) {
      MX_WARN(("firmware dead on board %d, ignoring ioctl\n", es->is->id));
      return EIO;
    }
  }

  switch (cmd) {

  case MYRI_SNF_SET_ENDPOINT_RX:
  case MYRI_SNF_SET_ENDPOINT_RX_BH:
  {
    myri_snf_rx_params_t x;

    if (es->snf.es_type != MYRI_SNF_ES_FREE) {
      status = EINVAL;
      break;
    }
    if ((status = mx_copyin(in, &x, sizeof(x), es->is_kernel)))
      break;
    if ((status = myri_snf_rx_open(es, &x)))
      break;
    if ((status = mx_copyout(&x, out, sizeof(x), es->is_kernel)))
      myri_snf_rx_teardown(es);
  }
  break;

  case MYRI_SNF_SET_ENDPOINT_RX_RING:
  {
    myri_snf_rx_attach_t x;

    if (es->snf.es_type != MYRI_SNF_ES_FREE) {
      status = EINVAL;
      break;
    }
    if ((status = mx_copyin(in, &x, sizeof(x), es->is_kernel)))
      break;
    if ((status = myri_snf_rx_attach(es, &x)))
      break;
    if ((status = mx_copyout(&x, out, sizeof(x), es->is_kernel)))
      myri_snf_rx_teardown(es);
  }
  break;

  case MYRI_SNF_SET_ENDPOINT_TX:
#if !MAL_OS_LINUX
    status = ENOTSUP;
#else
  {
    myri_snf_tx_params_t x;

    if (es->snf.es_type != MYRI_SNF_ES_FREE) {
      status = EINVAL;
      break;
    }
    if ((status = mx_copyin(in, &x, sizeof(x), es->is_kernel)))
      break;
    if ((status = myri_snf_tx_open(es, &x)))
      break;
    status = mx_copyout(&x, out, sizeof(x), es->is_kernel);
  }
#endif
  break;

  case MYRI_SNF_RX_START:
  case MYRI_SNF_RX_STOP:
    if (es->snf.es_type != MYRI_SNF_ES_RX) {
      status = EINVAL;
      break;
    }
    mx_mutex_enter(&snf->sync);
    if (cmd == MYRI_SNF_RX_START) {
      if (snf->state != MYRI_SNF_RX_S_STARTED) {
        status = myri_snf_start_stop(es, 1);
        snf->state = MYRI_SNF_RX_S_STARTED;
        if (snf->kagent.es) {
          myri_snf_rx_wake(snf, snf->kagent.es);
        }
      } else {
        status = 0;
      }
    }
    else {
      if (snf->state == MYRI_SNF_RX_S_STARTED) {
        status = myri_snf_start_stop(es, 0);
        snf->state = MYRI_SNF_RX_S_STOPPED;
      } else {
        status = 0;
      }
    }
    mx_mutex_exit(&snf->sync);
    break;

  case MYRI_SNF_RX_NOTIFY:
    {
      uint32_t x, notify_mask;
      int32_t i, ring_id;
      
      ring_id = es->snf.ring_id;

      status = mx_copyin(in, &x, sizeof(x), es->is_kernel);
      if (status)
        break;

      mx_mutex_enter(&snf->sync);
      notify_mask = x & snf->attach_mask;

      while ((i = snf__ffs(notify_mask)) > 0) {
        i--;
        notify_mask &= ~(1<<i);
        if (i != ring_id && snf->ring_map_ep[i]) {
          if (mx_wake_once(&snf->ring_map_ep[i]->wait_sync))
            snf->rx_s->vrings[i].vr_p.qnotifies++;
        }
      }
      mx_mutex_exit(&snf->sync);
    }
    break;

  case MYRI_SNF_WAIT:
    {
      uint32_t x;
      int events;
      status = mx_copyin(in, &x, sizeof(x), es->is_kernel);
      if (status)
        break;

      if (es->snf.es_type == MYRI_SNF_ES_TX)
        events = MYRI_WAIT_TX_EVENT;
#if MYRI_SNF_KAGENT
      else if (snf->kagent.state != MYRI_SNF_KAGENT_DISABLED) {
        if (snf->kagent.es == es)
          events = MYRI_WAIT_RX_EVENT;
        else {
          events = 0;
          /* Try to see if it's worth reaping some completions */
          myri_snf_rx_kagent_progress(snf, 0);
        }
      }
#endif
      else
        events = MYRI_WAIT_RX_EVENT;
      status = myri_poll_wait(es, x, events);
    }
    break;

  case MYRI_SNF_STATS:
    {
      myri_snf_stats_t x;
      myri_snf_get_stats(es->is, &x);
      status = mx_copyout(&x, out, sizeof(x), es->is_kernel);
    }
    break;

  case MYRI_SNF_RX_RING_PARAMS:
    {
      myri_snf_rx_ring_params_t x;
      if (es->snf.es_type != MYRI_SNF_ES_RX_RING && es->snf.es_type != MYRI_SNF_ES_RX)
        status = EINVAL;
      else {
        status = mx_copyin(in, &x, sizeof(x), es->is_kernel);
        if (status)
          ;
        else if (x.ring_id >= snf->params.num_rings)
          status = EINVAL;
        else if (!(snf->attach_mask & (1<<x.ring_id)))
          status = EAGAIN;
        else
          status = mx_copyout(&snf->rx_ring_cbs[x.ring_id].p, out, sizeof(x), es->is_kernel);
      }
    }
    break;

#if MAL_OS_LINUX || MAL_OS_FREEBSD
  case MYRI_SNF_SOFT_RX:
    {
      myri_soft_rx_t x;
      status = mx_copyin(in, &x, sizeof(x), es->is_kernel);
      if (status)
       break;
      status = myri_ether_soft_rx(es->is, &x);
    }
    break;
#endif /* MAL_OS_LINUX */
  }

  return status;
}

#if MYRI_SNF_KAGENT
/*
 * Given an es endpoint, duplicate it at as a kernel-level endpoint
 */
static
int
myri_snf_rx_kagent_start(myri_snf_rx_state_t *snf, const myri_snf_rx_params_t *x)
{
  mx_instance_state_t *is = snf->is;
  myri_snf_rx_params_t dparams;
  myri_snf_rx_attach_t ap;
  struct snf__rx *rx = NULL;
  struct snf__params *p = &snf->kagent.p;
  unsigned long flags;
  mx_endpt_state_t *es = NULL;
  int rc;

  flags = 0;  /* useless initialization to pacify -Wunused */

  p->debug_mask = 0x1;
  p->boardnum = is->id;
  p->epid = SNF_ENDPOINT_RX_BH;
  p->ring_id = SNF_RING_KAGENT;
  p->rss_hash_fn = snf__rss_hash;
  p->rss_context = (void *)(uintptr_t)x->rss_flags;

  rx = mx_kmalloc(sizeof(struct snf__rx), MX_WAITOK|MX_MZERO);
  if (rx == NULL) {
    MX_WARN(("No memory for snf__rx kagent\n"));
    return ENOMEM;
  }

  dparams = *x;
  /* from requested */
  if ((rc = myri_klib_set_endpoint(&es, p->boardnum, 0, 
                                   &dparams, MYRI_SNF_SET_ENDPOINT_RX_BH))) {
    MX_WARN(("kagent set RX endpoint: %d\n", rc));
    mx_kfree(rx);
    return ENOMEM;
  }

  mx_mutex_enter(&snf->sync);
  mal_assert(snf->kagent.state == MYRI_SNF_KAGENT_STARTING);
  snf->kagent.rx = rx;
  myri_snf_rx_get_kva_maps(es, &ap);
  mx_mutex_exit(&snf->sync);

  ap.params = dparams;
  ap.ring_id = SNF_RING_KAGENT;
  mx_spin_lock_irqsave(&is->clk_spinlock, flags);
  ap.nticks = is->clk_nticks;
  mx_spin_unlock_irqrestore(&is->clk_spinlock, flags);

  if ((rc = snf__rx_init(rx, es, p, &ap))) {
    MX_WARN(("kagent_rx_init: %d\n", rc));
    goto fail;
  }

  /* Start kagent */
  if ((rc = myri_snf_kagent_init(snf)))
    goto fail;

  return 0;

fail:
  if (es)
    mal_close(es);

  return rc;
}

void
myri_snf_rx_kagent_progress(myri_snf_rx_state_t *snf, int work_queue)
{
  int tries = 10;

  if (!work_queue) {
    if (mx_mutex_try_enter(&snf->kagent.sync))
      return;
  }
  else
    mx_mutex_enter(&snf->kagent.sync);

  while (tries--) {
    /* Process what we can and re-request interrupt */
    if (!myri_snf_recv_agent_process(snf->kagent.rx, work_queue)) {
      break;
    }
  }
  mx_mutex_exit(&snf->kagent.sync);
  if (work_queue)
    myri_snf_intr_req(snf->kagent.es, MYRI_WAIT_RX_EVENT);
}

static int 
myri_snf_rx_kagent_stop(myri_snf_rx_state_t *snf)
{
  mx_endpt_state_t *es = snf->kagent.es;

  /* TODO: rename to arch_fini */
  myri_snf_kagent_fini(snf);
  mal_close(es);
  return 0;
}
#endif
