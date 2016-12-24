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
 * Copyright 2008 - 2010 by Myricom, Inc.  All rights reserved.
 ***********************************************************************/

#ifndef _myri_snf_io_h
#define _myri_snf_io_h

#include "mal_io.h"
#include "mcp_types.h"
#include "mcp_requests.h"
#include "mal_piocopy.h"
#include "mal_byteswap.h"

/*
 * Tunables
 */
#define MYRI_SNF_MAX_RINGS  32

#define SNF_RSS_FLAGS_DEFAULT         (SNF_RSS_IP|SNF_RSS_SRC_PORT|SNF_RSS_DST_PORT)
#define SNF_DATARING_SZ_DEFAULT       (256ULL<<20)
#define SNF_MTUWRAP_SIZE              65536

/* 
 * Non-tunables
 */
#define SNF_VPAGE_SIZE 4096
#define SNF_VPAGE_SHIFT 12

#define SNF_RPD_ALIGN 16
#define SNF_RPD_MASK (SNF_RPD_ALIGN-1)
#define SNF_RPD_MAXLENGTH 16384

#define SNF_VTOKEN_SIZE 16
#define SNF_RSS_FLAGS_CUSTOM_FUNC 0x80000000

#define SNF_RING_UNUSED -1
#define SNF_RING_KAGENT -2

#define SNF_ENDPOINT_RX -3
#define SNF_ENDPOINT_RX_RING -4
#define SNF_ENDPOINT_RX_BH -5

#define MYRI_SNF_F_RX_COPY  0x100 /* Same as SNF_F_RX_PRIVATE */
#define MYRI_SNF_F_RX_DUP   0x200
#define MYRI_SNF_F_RX_KAGENT_ENABLE  0x1000
#define MYRI_SNF_F_RX_KAGENT_DISABLE 0x2000
#define MYRI_SNF_F_RX_KAGENT_DEFAULT MYRI_SNF_F_RX_KAGENT_DISABLE
/* SNF_F_RX_DUPLICATE at API level is a mask of RX_COPY|RX_DUP */

#define MYRI_SNF_RXMAP_PQ_DESC 0x1
#define MYRI_SNF_RXMAP_PQ_DATA 0x2
#define MYRI_SNF_RXMAP_VQ_DESC 0x4
#define MYRI_SNF_RXMAP_VQ_DATA 0x8

#define MYRI_OFFSET_UNMAPPED -1

#define SNF__CACHEALIGN __attribute__((aligned(64)))
#define SNF__PIOALIGN   __attribute__((aligned(8)))

#define SNF_ALIGNDOWN(p,P) (((uint64_t)(uintptr_t)(p))&~((uint64_t)(uintptr_t)((P)-1)))
#define SNF_ALIGNUP(p,P)   (SNF_ALIGNDOWN((uint64_t)(uintptr_t)(p)+((uint64_t)(uintptr_t)((P)-1)),P))

#define snf_assert mal_assert
#define snf_always_assert mal_always_assert

#ifndef MAL_KERNEL
#undef mal_assertion_failed
#define mal_assertion_failed snf__assertion_failed
void snf__assertion_failed(const char *assertion, int line, const char *file);
#endif

#define SNF__CTA(x)						\
  do								\
    {								\
      char (*a)[(__builtin_constant_p(x) && (x)) ? 1 : -1] = 0;	\
      (void) a;							\
    }								\
  while (0)

#define SNF_IS_POWER_OF_TWO(x) (((x) & ((x) - 1)) == 0)

#define SNF_DEBUGM_ERROR    0x0
#define SNF_DEBUGM_WARN     0x1
#define SNF_DEBUGM_PARAM    0x2
#define SNF_DEBUGM_QSTATS   0x4
#define SNF_DEBUGM_TIMESYNC 0x8
#define SNF_DEBUGM_IOCTL    0x10
#define SNF_DEBUGM_QEVENTS  0x20

#define SNF_DEBUGM_LABELS { "WARN", "PARAM", "QSTATS", "TIMESYNC", "IOCTL", "QEVENTS" }

#define SNF_DEBUGM_PREFIX_WARN ' '
#define SNF_DEBUGM_PREFIX_ERROR 'E'
#define SNF_DEBUGM_PREFIX_PARAM 'P'
#define SNF_DEBUGM_PREFIX_QSTATS 'S'
#define SNF_DEBUGM_PREFIX_TIMESYNC 'T'
#define SNF_DEBUGM_PREFIX_IOCTL 'I'
#define SNF_DEBUGM_PREFIX_QEVENTS 'Q'

#ifdef MAL_KERNEL
#define _SNF_DPRINTF(p,level,format,...)  do {                  \
        if (((p)->debug_mask & SNF_DEBUGM_ ## level) || SNF_DEBUGM_ERROR == SNF_DEBUGM_ ## level) \
            MX_PRINT(("snf.%d.%d %c " format,   \
                    (p)->boardnum, (p)->ring_id, SNF_DEBUGM_PREFIX_ ## level, \
                    __VA_ARGS__));                              \
        } while (0)
#else
#define _SNF_DPRINTF(p,level,format,...)  do {                  \
        if (((p)->debug_mask & SNF_DEBUGM_ ## level) || SNF_DEBUGM_ERROR == SNF_DEBUGM_ ## level) \
            fprintf((p)->debug_fp ? (p)->debug_fp : stderr, "snf.%d.%d %c " format,      \
                    (p)->boardnum, (p)->ring_id, SNF_DEBUGM_PREFIX_ ## level, \
                    __VA_ARGS__);                               \
        } while (0)
#endif

#define SNF_DPRINTF(params,level,format,...) \
            _SNF_DPRINTF(params,level,format,__VA_ARGS__)


struct snf_recv_req;
typedef int (*snf_rss_fn_t)(struct snf_recv_req *, void *, uint32_t *);

#ifndef MAL_KERNEL
#include <stdio.h> /* FILE */
#endif
struct snf__params {
  uint32_t     debug_mask;
  uint32_t     boardnum;
  uint32_t     epid;
  int          ring_id;
  snf_rss_fn_t rss_hash_fn;
  void        *rss_context;
#ifndef MAL_KERNEL
  const char  *debug_filename;
  FILE        *debug_fp;
  char         debug_file_buf[128];
#endif
};

struct snf__rx_state {
  struct {
    uint32_t reclaim_mask;
    uint32_t ring_id;
  } pq SNF__CACHEALIGN;

  struct snf__eventq_state  {
    uint32_t evq_seqnum;
    uint32_t evq_flow_desc;
    uint32_t evq_cur_idx;
    uint32_t evq_last_idx;
    const mcp_snf_rx_desc_t *evq_last_elem;
    const mcp_snf_rx_desc_t *evq_cur_elem;
  } evq;

  struct snf__recvq_state {
    uint64_t rq_data_off;
    uint64_t rq_data_recvd;
    uint64_t rq_data_returned;
    uint64_t rq_data_size;
    uint64_t rq_dataq_c;
    uint64_t rq_pkt_cnt;
    uint64_t rq_ev_ts_last;
    uint64_t rq_neg_adj_pkt;
    uint64_t rq_neg_adj_event;
    uint64_t rq_rss_drops;

    struct snf__eventq_evinfo {
      uint64_t ev_timestamp;
      uint32_t ev_length;
      uint32_t ev_length_time;
      uint32_t ev_pkt_cnt;
      uint32_t ev_pkt_idx;
      uint32_t ev_pkt_off;
      uint16_t ev_pkt_lens[SNF_RX_DESC_PKT_MAX];
    } ev_info;
  } recvq;

  struct snf__flow_state {
    uint32_t flow_desc;
    uint32_t flow_data;
  } flow_rx;

  struct snf__vrx_state {
    struct {
      uint64_t pkt_recvd;
      uint64_t pkt_drops;
      uint64_t pkt_drops_data;
      uint64_t pkt_drops_rss;
      uint64_t qnotifies;
      uint64_t dataq_off;
      uint32_t seq;
      uint32_t iter;
    } vr_p  SNF__CACHEALIGN;
    struct {
      uint64_t dataq_off;
      uint64_t qwaits;
      uint32_t seq;
      uint32_t iter;
    } vr_c  SNF__CACHEALIGN;
  } vrings[MYRI_SNF_MAX_RINGS] SNF__CACHEALIGN;
};

typedef struct { volatile uint32_t slock; } snf__spinlock_t;

struct snf__tx_state {
  myri_endpt_flow_t ep_flow SNF__CACHEALIGN;

  snf__spinlock_t tx_lock;

  uintptr_t dq_avail;
  uint8_t  dq_head_idx;
  uint8_t  pad;
  int16_t  dq_inflight;
  uint16_t dq_lengths[SNF_TX_DESC_MAX];

  uint64_t tx_pkts_cnt;
  uint64_t tx_flush_cnt;
  uint64_t tx_flush_intr_cnt;

  struct snf__tx_desc_state {
    uint32_t d_cnt;
    uint32_t d_length;
    uint16_t d_pkt_lens_be16[SNF_TX_DESC_PKT_MAX] SNF__PIOALIGN;
  } txd;
};

struct snf__tx_shared_params {
  uint32_t  txd_cnt;
  uint32_t *txd_dbaddr_be32;
  uint8_t  *txd_compl_update;
  uintptr_t txd_pio_base;
  struct snf__tx_state *tx_s;
};

static inline
void
snf__rx_flow_update(uint64_t *rx_flow_be64, const struct snf__flow_state *flowh)
{
  struct {
    uint32_t flow_desc_be32;
    uint32_t flow_data_be32;
  } flow;
  flow.flow_desc_be32 = htonl(flowh->flow_desc);
  flow.flow_data_be32 = htonl(flowh->flow_data);
  *rx_flow_be64 = *((uint64_t *)&flow);
  MAL_STBAR();
  return;
}

static inline
int snf__ffs(int x)
{
  int r;
  __asm__("bsfl %1,%0\n\t"
	  "cmovzl %2,%0" 
	  : "=r" (r) : "rm" (x), "r" (-1));
  return r+1;
}

static inline
uint32_t snf__cmpswap_u32(uint32_t *p, uint32_t oldv, uint32_t newv)
{
  __asm__("lock\n\t"
          "cmpxchgl %1,%2"
          : "=a" (oldv) : "q" (newv), "m" (*p), "0" (oldv) : "memory");
  return oldv;
}

static inline
int snf__spin_trylock(snf__spinlock_t *lock)
{
  int oldv;

  __asm__ __volatile__("xchgl %0, %1"
                       :"=q" (oldv), "=m" (lock->slock)
                       :"0" (0) : "memory");
  return oldv > 0;
}

static inline
void snf__spin_init(snf__spinlock_t *lock)
{
  lock->slock = 1;
}

static inline
void snf__spin_lock(snf__spinlock_t *lock)
{
  while (!snf__spin_trylock(lock))
       __asm__ __volatile__("rep;nop": : :"memory");
}

static inline
void snf__spin_unlock(snf__spinlock_t *lock)
{
  __asm__ __volatile__("movl $1, %0"
                       :"=m" (lock->slock)
                       : : "memory");
}

static inline
void
snf__tx_pioflush(struct snf__tx_shared_params *p)
{
  struct snf__tx_state *ss = p->tx_s;
  size_t piolen;
  uint32_t fdata;
  uintptr_t pioaddr;
  uint8_t idx;

  mal_assert(ss->txd.d_cnt > 0);
  mal_assert(ss->txd.d_length > 0);

  idx = ss->dq_head_idx & (p->txd_cnt - 1);
  ss->dq_head_idx++;
  ss->dq_lengths[idx] = ss->txd.d_length;

  piolen = (ss->txd.d_cnt * sizeof(uint16_t) + 7) & ~7;
  pioaddr = p->txd_pio_base + idx * SNF_TX_DESC_PKT_MAX * sizeof(uint16_t);
  mal_piocopy64_inline((uint64_t *) pioaddr,
                       (uint64_t *) ss->txd.d_pkt_lens_be16, piolen, 0);

  fdata = ((uint32_t) ss->txd.d_length << 16) | ss->txd.d_cnt;
  ss->tx_pkts_cnt += ss->txd.d_cnt;
  ss->txd.d_length = 0;
  ss->txd.d_cnt = 0;
  ss->dq_inflight++;
  ss->tx_flush_cnt++;
  *p->txd_dbaddr_be32 = htonl(fdata);
  MAL_STBAR();
}

static inline
int
snf__tx_update_completion(struct snf__tx_shared_params *tx_sp)
{
  struct snf__tx_state *ss = tx_sp->tx_s;
  uint8_t tx_compl_host, tx_compl_nic, idx;
  uint32_t d_length;
  int done = 0;

  /* reclaim all requests that are complete right now */
  tx_compl_nic = *tx_sp->txd_compl_update;
  tx_compl_host = (uint8_t) ss->ep_flow.tx_flow.flow_desc;
  if (tx_compl_nic == tx_compl_host)
    return 0; /* nothing */

  while (tx_compl_host != tx_compl_nic) {
    ss->dq_inflight--;
    mal_assert(ss->dq_inflight >= 0);
    idx = tx_compl_host & (uint8_t)(tx_sp->txd_cnt-1);
    d_length = ss->dq_lengths[idx];
    mal_assert(d_length != 0);
    ss->dq_avail += SNF_ALIGNUP(d_length, SNF_VPAGE_SIZE);
    ss->dq_lengths[idx] = 0;
    tx_compl_host++;
    done++;
  }

  /* Write updated value on host page */
  ss->ep_flow.tx_flow.flow_desc = tx_compl_host;
  return done;
}

#endif /* _myri_snf_io_h */
