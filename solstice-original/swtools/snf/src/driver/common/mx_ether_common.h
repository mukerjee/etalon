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

#ifndef _mx_ether_common_h_
#define _mx_ether_common_h_

#include "mal_auto_config.h"

#define NUM_TX	((unsigned long)(MX_ETHER_TX_SIZE) / sizeof(mcp_kreq_ether_send_t))
#define NUM_RX	((unsigned long)(MX_ETHER_RX_SIZE) / sizeof(mcp_dma_addr_t))

#define MX_MAX_ETHER_MTU 9014
#if MYRI_ENABLE_PTP
#include "myri_ptp_common.h"
#endif

#define MX_ETH_STOPPED 0
#define MX_ETH_STOPPING 1
#define MX_ETH_STARTING 2
#define MX_ETH_RUNNING 3
#define MX_ETH_OPEN_FAILED 4

#include "mx_ether.h"
#include "mx_pio.h"
#include "mal_io.h"

typedef struct
{
	int cnt;
	int alloc_fail;
#if MAL_OS_LINUX
	int fill_cnt;
	int page_offset;
	int watchdog_needed;
	struct page *page;
	dma_addr_t bus;	
#endif
	uint32_t *lanai_cnt;	/* lanai counter	*/
	mcp_dma_addr_t *ring;	/* lanai ptr for recvq	*/
	mcp_dma_addr_t *shadow;	/* shadow of MCP ring	*/
	struct mx_ether_buffer_info info[NUM_RX];
#if MAL_OS_WINDOWS
	NDIS_HANDLE nbl_pool_handle;
	struct mx_ether_buffer_info *pinfo[NUM_RX];
#endif
} mx_ether_rx_buf_t;

typedef struct
{
	mx_copyblock_t cb;
	unsigned int cnt;
} mx_ether_rx_desc_t;

typedef struct
{
	int req;			/* transmits submitted	*/
	uint32_t *lanai_cnt;		/* lanai counter	*/
	mcp_kreq_ether_send_t *ring;	/* lanai ptr for sendq	*/
	struct mx_ether_buffer_info info[NUM_TX];
	int done;			/* transmits completed	*/

#if MAL_OS_WINDOWS
	int avail;                      /* transmits available */
	NDIS_SPIN_LOCK sg_lock; /* Serialize alloc and free */
	int sgl_stack_top;
	char **sgl_stack;     /* Free SGLs */
	int sgl_size;         /* Size of each SGL */
	int sgl_total_count;  /* Total number of SGLs */
#endif
#if MAL_OS_SOLARIS
	struct myri_ether_tx_copybuf *cp;
#endif
} mx_ether_tx_buf_t;

struct mx_ether {
	struct mx_ether_arch arch;	/* OS dependent 	*/
	int running;			/* running? 		*/
	int csum_flag;			/* rx_csums? 		*/
	int pause;			/* flow-control?        */
	mx_instance_state_t *is;	/* Hardware state 	*/
	uint8_t	current_mac[6];		/* software mac address */
	mx_ether_tx_buf_t tx;		/* transmit ring 	*/
	mx_ether_rx_buf_t rx_small;
	mx_ether_rx_buf_t rx_big;
	mx_ether_rx_desc_t rx_desc;
	mx_spinlock_t spinlock;
	mx_sync_t down_sync;
};

int  mx_ether_open_common(mx_instance_state_t *is, int mtu, 
			  int max_small_rx, int max_big_rx);
void mx_ether_close_common(mx_instance_state_t *is);
int mx_ether_set_promisc_common(struct mx_ether *sc, int on);
void mx_ether_set_mac_address_common(struct mx_ether *sc, uint8_t *addr);
int  mx_ether_set_pause_common(struct mx_ether *eth, int pause);
void mx_ether_start_common(mx_instance_state_t *is, int mtu,  
			   int max_small_rx, int max_big_rx);

void mx_ether_tx_done(mx_instance_state_t *is, uint32_t mcp_index);
void mx_ether_rx_done_small(mx_instance_state_t *is, mcp_ether_rx_desc_t *desc);
void mx_ether_rx_done_big(mx_instance_state_t *is, mcp_ether_rx_desc_t *desc);
int mx_ether_attach(mx_instance_state_t *is);
void mx_ether_detach(mx_instance_state_t *is);
int mx_ether_parity_detach(mx_instance_state_t *is);
void mx_ether_parity_reattach(mx_instance_state_t *is);
void mx_ether_link_change_notify(mx_instance_state_t *is);
void mx_ether_lro_flush(mx_instance_state_t *is);
void mx_ether_watchdog(mx_instance_state_t *is);
int myri_ether_soft_rx(mx_instance_state_t *is, myri_soft_rx_t *r);

/* copy an array of mcp_kreq_ether_send_t's to the mcp and safely
   update the sendq counter */

static inline void
mx_ether_submit_tx_req(struct mx_ether *eth, mcp_kreq_ether_send_t *src, 
		       int cnt)
{
	int req, frag, idx;
	
	mal_always_assert (sizeof(mcp_kreq_ether_send_t) == 16);
	req = eth->tx.req;
	for (frag = 0; frag < cnt; frag++) {
		idx = req & (NUM_TX - 1);
		req++;
		mx_pio_memcpy(&eth->tx.ring[idx], &src[frag], 
			    sizeof (*src), 0);
		if (frag & 1)
			MAL_STBAR();
	}
	eth->tx.req = req;
	MAL_STBAR();
	
	/* update the lanai sendq counter */
	MX_PIO_WRITE(eth->tx.lanai_cnt, htonl(req));
	MAL_STBAR();
}

#define mx_ether_gw_rx(is, pkt)
#define mx_ether_gw_tx(is, req, pkt)
#define myri_ptp_store_rx_timestamp(is, pkt, timestamp_ns)

#if MYRI_ENABLE_PTP
#undef myri_ptp_store_rx_timestamp
void myri_ptp_store_rx_timestamp(struct mx_instance_state *is, uint8_t *pkt, uint64_t timestamp_ns);
#endif


static inline void 
mx_ether_rx_done(mx_instance_state_t *is)
{
	struct mx_ether *eth;
	mcp_ether_rx_desc_t *desc;
	unsigned int cnt, page, offset;
	
#ifdef HAVE_NEW_NAPI
	if (MAL_IS_ZE_BOARD(is->board_type)) {
	  napi_schedule(&is->ether->arch.napi);
	  return;
	}
#endif
	eth = is->ether;
	
	do {
	  cnt = eth->rx_desc.cnt & ((MX_ETHER_RX_DESC_VPAGES * MX_VPAGE_SIZE
				     / sizeof(mcp_ether_rx_desc_t)) - 1);
	  page = cnt * sizeof(mcp_ether_rx_desc_t) / MX_PAGE_SIZE;
	  offset = (cnt * sizeof(mcp_ether_rx_desc_t)) & (MX_PAGE_SIZE - 1);
	  desc = (mcp_ether_rx_desc_t *)((uintptr_t) eth->rx_desc.cb.pins[page].va + offset);
	  
	  if (desc->type == 0)
	    goto out;

	  if (desc->type == MCP_ETHER_RX_SMALL)
	    mx_ether_rx_done_small(is, desc);
	  else
	    mx_ether_rx_done_big(is, desc);
	  
	  desc->type = 0;
	  eth->rx_desc.cnt++;
	} while (1);

out:
	if (MAL_IS_ZE_BOARD(is->board_type)) {
	  MX_PIO_WRITE((uint32_t*)(is->lanai.sram + 0x6c0000), 
		       htonl(eth->rx_desc.cnt));
	}
}

#endif /* _mx_ether_common_h_ */

/*
  This file uses MX driver indentation.

  Local Variables:
  c-file-style:"linux"
  tab-width:8
  End:
*/
