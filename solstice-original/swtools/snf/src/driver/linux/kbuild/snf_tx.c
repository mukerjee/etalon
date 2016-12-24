/*************************************************************************
 * The contents of this file are subject to the MYRICOM SNIFFER10G
 * LICENSE (the "License"); User may not use this file except in
 * compliance with the License.  The full text of the License can found
 * in LICENSE.TXT
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and
 * limitations under the License.
 *
 * Copyright 2010 by Myricom, Inc.  All rights reserved.
 ***********************************************************************/

#include "myri_version.h"
#include "mcp_config.h"
#include "snf_libtypes.h"
#include "snf.h"
#include "snf_tx.h"
#include "mal_piocopy.h"

static
int
open_tx_endpoint(uint32_t boardnum, myri_snf_tx_params_t *txp,
                 mal_handle_t *handle_o)
{
  int rc;
  mal_handle_t handle;
  uint32_t epid, max_ep;

  if ((rc = mal_open_any_board(&handle)))
    return rc;

  if ((rc = mal_ioctl(handle, MYRI_GET_ENDPT_MAX, &max_ep, sizeof(max_ep)))) {
    mal_close(handle);
    return rc;
  }

  if ((rc = mal_close(handle)))
    return rc;

  for (epid = 0; epid < max_ep; epid++) {
    if (!(rc = mal_open(boardnum, epid, &handle))) {
      txp->epid = epid;
#ifdef MAL_KERNEL
      rc = myri_klib_set_endpoint(&handle, boardnum, epid, txp,
                                  MYRI_SNF_SET_ENDPOINT_TX);
#else
      rc = mal_ioctl(handle, MYRI_SNF_SET_ENDPOINT_TX, txp, sizeof(*txp));
#endif
      if (rc) {
        mal_close(handle);
      }
      else {
        *handle_o = handle;
        break;
      }
    }
  }
  return rc;
}

#ifdef MAL_KERNEL
#define snf__mmap(p,handle,map_name,map_ptr,size,offset,ro)  \
          myri_klib_map(handle, offset, (void **) map_ptr, NULL)
#endif

static
int
snf__tx_map(const struct snf__params *p, struct snf__tx_map *map,
            mal_handle_t handle, const myri_snf_tx_params_t *dparams)
{
  int rc = 0;

  /* Map tx data ring */
  map->map_tx_data_size = (uintptr_t) dparams->tx_data_ring_size;
  if ((rc = snf__mmap(p, handle, "tx_data", &map->map_tx_data,
                      map->map_tx_data_size, dparams->tx_data_ring_offset, 0))) 
    return rc;

  /* Map tx desc ring */
  map->map_tx_desc_size = (uintptr_t) dparams->tx_desc_ring_size;
  if ((rc = snf__mmap(p, handle, "tx_desc", &map->map_tx_desc,
                      map->map_tx_desc_size, dparams->tx_desc_ring_offset, 0)))
    return rc;

  /* Map tx completion counts */
  map->map_tx_compl_size = (uintptr_t) dparams->tx_compl_size;
  if ((rc = snf__mmap(p, handle, "tx_compl", &map->map_tx_compl,
                      map->map_tx_compl_size, dparams->tx_compl_offset, 1)))
    return rc;

  /* Map tx NIC doorbell */
  map->map_tx_doorbell_size = (uintptr_t) dparams->tx_doorbell_size;
  if ((rc = snf__mmap(p, handle, "tx_doorbell", &map->map_tx_doorbell,
                      map->map_tx_doorbell_size, dparams->tx_doorbell_offset, 0)))
    return rc;

  /* Map tx flow with kernel */
  map->map_tx_flow_size = (uintptr_t) dparams->tx_flow_size;
  if ((rc = snf__mmap(p, handle, "tx_flow", &map->map_tx_flow,
                      map->map_tx_flow_size, dparams->tx_flow_offset, 0)))
    return rc;

  return rc;
}

static
void
snf__tx_unmap(struct snf__tx_map *map)
{
#ifndef MAL_KERNEL
  if (map->map_tx_data)
    mal_munmap((void *)map->map_tx_data, map->map_tx_data_size);
  if (map->map_tx_desc)
    mal_munmap((void *)map->map_tx_desc, map->map_tx_desc_size);
  if (map->map_tx_doorbell)
    mal_munmap((void *)map->map_tx_doorbell, map->map_tx_doorbell_size);
  if (map->map_tx_compl)
    mal_munmap((void *)map->map_tx_compl, map->map_tx_compl_size);
  if (map->map_tx_flow)
    mal_munmap((void *)map->map_tx_flow, map->map_tx_flow_size);
#endif
}

int 
snf__tx_init(struct snf__tx *tx, mal_handle_t mhandle,
             const struct snf__params *p, const myri_snf_tx_params_t *dparams)
{
  int rc = 0;
  struct snf__tx_state *ss;

  memset(tx, 0, sizeof(*tx));
  tx->mhandle = mhandle;

  if ((rc = snf__tx_map(p, &tx->tx_map, mhandle, dparams)))
    return rc;

  /** for shared pio */
  tx->tx_sp.txd_cnt = MX_VPAGE_SIZE / (SNF_TX_DESC_PKT_MAX * 2);
  tx->tx_sp.txd_dbaddr_be32 = (uint32_t *) tx->tx_map.map_tx_doorbell;
  tx->tx_sp.txd_pio_base = tx->tx_map.map_tx_desc;
  tx->tx_sp.tx_s = ss = (struct snf__tx_state *) tx->tx_map.map_tx_flow;
  tx->tx_sp.txd_compl_update = (uint8_t *) tx->tx_map.map_tx_compl + p->epid;
  snf_always_assert(SNF_IS_POWER_OF_TWO(tx->tx_sp.txd_cnt));

  tx->txdata_offset = 0;
  tx->txdata_size = tx->tx_map.map_tx_data_size;

  snf__spin_init(&ss->tx_lock);
  ss->dq_avail = tx->txdata_size;
  ss->tx_flush_cnt = 0;
  ss->tx_flush_intr_cnt = 0;

  rc = mal_ioctl(mhandle, MYRI_SNF_STATS, &tx->tx_stats, sizeof(tx->tx_stats));
  if (rc != 0)
    snf__tx_unmap(&tx->tx_map);

  return 0;
}

MAL_FUNC(int)
snf_inject_open(int boardnum, int flags, snf_inject_t *inj_o)
{
  snf_inject_t handle;
  int rc;

  /* Must have called snf_init */
  if (!snf__init) {
    return EINVAL;
  }

  if (!(handle = mal_calloc(1, sizeof(*handle)))) {
    rc = ENOMEM;
    goto bail;
  }

#ifndef MAL_KERNEL
  {
    struct snf_param_keyval op;
    op.key = SNF_PARAM_BOARDNUM;
    op.val.boardnum = boardnum;

    if ((rc = snf__api_params(&handle->p, NULL, &op, 1, NULL)))
      goto bail;
  }
#else
  handle->p.boardnum = boardnum;
  handle->p.debug_mask = 0x1;
#endif

  handle->mhandle = MAL_INVALID_HANDLE;

  if ((rc = open_tx_endpoint(boardnum, &handle->drv_p, &handle->mhandle))) {
    SNF_DPRINTF(&handle->p, IOCTL, 
      "Can't open snf inject handle on board %u (err=%d)\n", boardnum, rc);
    goto bail;
  }
  handle->p.epid = handle->drv_p.epid;

  rc = snf__tx_init(&handle->tx, handle->mhandle, &handle->p, &handle->drv_p);

bail:
  if (rc) {
    if (handle) {
      if (handle->mhandle != MAL_INVALID_HANDLE)
        mal_close(handle->mhandle);
      mal_free(handle);
    }
  }
  else
    *inj_o = handle;

  return rc;
}

static inline
int
tx_wait_event(struct snf__tx *tx, int timeout_ms)
{
  return mal_ioctl(tx->mhandle, MYRI_SNF_WAIT, &timeout_ms, sizeof(timeout_ms));
}

MAL_FUNC(int)
snf_inject_send(snf_inject_t inj, int timeout_ms, const void *pkt, uint32_t length)
{
  struct snf__tx *tx = &inj->tx;
  struct snf__tx_state *ss = tx->tx_sp.tx_s;
  uintptr_t data_off, data_off_next;
  uint32_t copylen, length_up, length_pq;
  int rc = 0;

  if (length > SNF__TX_MAX_MTU)
    return EINVAL;

  snf__spin_lock(&ss->tx_lock);
  length_pq = length_up = SNF_ALIGNUP(length, SNF_TX_DESC_PKT_ALIGN);

  data_off = tx->txdata_offset;

  if (ss->txd.d_cnt == 0 ||
      ss->txd.d_length + length_up >= SNF__TX_DESC_MAX_FLUSH_LEN) 
  {
    if (ss->txd.d_cnt) /* previous descriptor is big enough */
      snf__tx_pioflush(&tx->tx_sp);

    /* New descriptor */
    data_off = SNF_ALIGNUP(data_off, SNF_VPAGE_SIZE);
    length_pq += data_off - tx->txdata_offset;
    if (unlikely(data_off == tx->txdata_size))
      data_off = 0;

    if (ss->dq_inflight == tx->tx_sp.txd_cnt) {
      if (timeout_ms == 0)
        rc = EAGAIN;

      do {
        if (!snf__tx_update_completion(&tx->tx_sp)) {
          if (rc == EAGAIN || ((rc = tx_wait_event(tx, timeout_ms)) && rc != EAGAIN))
            goto bail_with_lock;
        }
      } while (ss->dq_inflight == tx->tx_sp.txd_cnt);
    }
    rc = 0;
  }

  if (ss->dq_avail < length_pq) {
    if (timeout_ms == 0)
      rc = EAGAIN;
    do {
      if (!snf__tx_update_completion(&tx->tx_sp)) {
        if (rc == EAGAIN || ((rc = tx_wait_event(tx, timeout_ms)) && rc != EAGAIN))
          goto bail_with_lock;
      }
    }
    while (ss->dq_avail < length_pq);
  }
  rc = 0;

  copylen = length;
  data_off_next = data_off + length_up;
  if (unlikely(data_off_next >= tx->txdata_size)) {
    uintptr_t data_end = data_off + length;
    if (data_end > tx->txdata_size) {
      uint32_t nobx = (uint32_t)(data_end - tx->txdata_size);
      /* Copy the packet's trailing portion at beginning of the ring and let
       * the regular code handle the part of the packet that doesn't overspill
       */
      memcpy((void *)(tx->tx_map.map_tx_data),
             (void *)((uintptr_t) pkt + length - nobx), (uint32_t) nobx);
      copylen = length - (uint32_t) nobx;
    }
    data_off_next -= tx->txdata_size;
  }

  memcpy((void *)(tx->tx_map.map_tx_data + data_off), pkt, copylen);
  ss->dq_avail -= length_up;
  tx->txdata_offset = data_off_next;

  ss->txd.d_length += length_up;
  ss->txd.d_pkt_lens_be16[ss->txd.d_cnt++] = htons(length);

  if (ss->dq_inflight == 0
      || ss->txd.d_cnt >= SNF_TX_DESC_PKT_MAX
      || ss->txd.d_length >= SNF__TX_DESC_MAX_FLUSH_LEN)
    snf__tx_pioflush(&tx->tx_sp);

bail_with_lock:
  snf__spin_unlock(&ss->tx_lock);
  return rc;
}

MAL_FUNC(int)
snf_inject_close(snf_inject_t handle)
{
  struct snf__tx *tx;
  struct snf__tx_state *ss;

  tx = &handle->tx;
  ss = tx->tx_sp.tx_s;

  snf__spin_lock(&ss->tx_lock);
  if (ss->txd.d_cnt) {
    snf__tx_pioflush(&tx->tx_sp);
    while (ss->dq_inflight) {
      snf__spin_unlock(&ss->tx_lock);
      tx_wait_event(tx, 10);
      snf__spin_lock(&ss->tx_lock);
    }
    snf__spin_unlock(&ss->tx_lock);
  }

  SNF_DPRINTF(&handle->p, PARAM,
    "inject_close: flushed %lld out of %lld, %4.2f %%, intr flush=%lld\n",
    (long long) ss->tx_flush_cnt, (long long) ss->tx_pkts_cnt,
    (double) ss->tx_flush_cnt * 100.0 /  ss->tx_pkts_cnt,
    (long long) ss->tx_flush_intr_cnt);

  snf__tx_unmap(&tx->tx_map);
  mal_close(handle->mhandle);
  mal_free(handle);

  return 0;
}

MAL_FUNC(int)
snf_inject_getstats(snf_inject_t inj, struct snf_inject_stats *stats)
{
  int rc;
  myri_snf_stats_t mstats;
  struct snf__tx_state *ss = inj->tx.tx_sp.tx_s;

  memset(&mstats, 0, sizeof(mstats));

  if ((rc = mal_ioctl(inj->mhandle, MYRI_SNF_STATS, &mstats, sizeof(mstats))))
    return rc;

  stats->inj_pkt_send = ss->tx_pkts_cnt;
  stats->nic_pkt_send = (mstats.nic_pkt_send - inj->tx.tx_stats.nic_pkt_send);
  stats->nic_bytes_send = (mstats.nic_bytes_send - inj->tx.tx_stats.nic_bytes_send);

  return 0;
}
