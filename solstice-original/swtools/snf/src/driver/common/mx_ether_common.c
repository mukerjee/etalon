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
#include "mx_ether_common.h"
#include "mx_pio.h"


static void
myri_ether_free_common(mx_instance_state_t *is)
{
	struct mx_ether *ether = is->ether;
	
	if (ether->rx_small.shadow) {
		mx_kfree(ether->rx_small.shadow);
		ether->rx_small.shadow = 0;
	}
	if (ether->rx_big.shadow) {
		mx_kfree(ether->rx_big.shadow);
		ether->rx_big.shadow = 0;
	}
	if (ether->rx_desc.cb.pins) {
		mx_free_copyblock(is, &ether->rx_desc.cb);
	}
}

static int
myri_ether_alloc_common(mx_instance_state_t *is)
{
	int status;
	struct mx_ether *ether = is->ether;
	
	/* allocate Ether rings */
	ether->rx_small.shadow = mx_kmalloc(MX_ETHER_RX_SIZE, MX_MZERO|MX_WAITOK);
	ether->rx_big.shadow = mx_kmalloc(MX_ETHER_RX_SIZE, MX_MZERO|MX_WAITOK);
	if (!ether->rx_small.shadow || !ether->rx_big.shadow) {
		MX_WARN(("%s: Ether rings allocation failed\n", is->is_name));
		myri_ether_free_common(is);
		return ENOMEM;
	}
	
	ether->rx_desc.cb.size = MX_ETHER_RX_DESC_VPAGES * MX_VPAGE_SIZE;
	status = mx_alloc_copyblock(is, &ether->rx_desc.cb);
	if (status) {
		MX_WARN(("%s: Ether copyblock allocation failed\n", is->is_name));
		myri_ether_free_common(is);
		return status;
	}
	
	return 0;
}


/* it is expected that a driver would call this before
   allocating its receive buffers */

int
mx_ether_open_common(mx_instance_state_t *is, int mtu, int max_small_rx, 
		     int max_big_rx)
{
	struct mx_ether *ether = is->ether;
	int status, i;
	
	/* set the static parameters */
	MCP_SETVAL(is, ethernet_bigbuf, max_big_rx);

	ether->tx.ring = (mcp_kreq_ether_send_t *) MCP_GETPTR(is, ether_tx_ring);
	ether->tx.lanai_cnt = (uint32_t *) MCP_GETPTR(is, ether_tx_cnt);
	ether->rx_small.ring = (mcp_dma_addr_t *) MCP_GETPTR(is, ether_rx_small_ring);
	ether->rx_small.lanai_cnt = (uint32_t *) MCP_GETPTR(is, ether_rx_small_cnt);
	ether->rx_big.ring = (mcp_dma_addr_t *) MCP_GETPTR(is, ether_rx_big_ring);
	ether->rx_big.lanai_cnt = (uint32_t *) MCP_GETPTR(is, ether_rx_big_cnt);
	ether->tx.req = 0;
	ether->tx.done = 0;
	ether->rx_big.cnt = 0;
	ether->rx_small.cnt = 0;
	ether->rx_desc.cnt = 0;
	
	/* clear the mcp's polling variables */
	MX_PIO_WRITE(ether->tx.lanai_cnt, 0);
	MX_PIO_WRITE(ether->rx_small.lanai_cnt, 0);
	MX_PIO_WRITE(ether->rx_big.lanai_cnt, 0);
	MAL_STBAR();

	status = myri_ether_alloc_common(is);
	if (status) {
		MX_WARN(("%s: ether alloc common failed, errno = %d\n",
			 is->is_name, status));
		goto abort_with_nothing;
	}
	
	for (i = 0; i < MX_ETHER_RX_DESC_VPAGES; i++) {
		MCP_SETVAL(is, ether_rx_desc_dma[i].high, 
			   ether->rx_desc.cb.pins[i].dma.high);
		MCP_SETVAL(is, ether_rx_desc_dma[i].low, 
			   ether->rx_desc.cb.pins[i].dma.low);
	}
	
	mx_spin_lock_init(&ether->spinlock, is, -1, "ether spinlock");
	mx_sync_init(&ether->down_sync, is, -1, "ether down sync");
	
	
	is->ether_is_open = 1;
	return 0;

abort_with_nothing:
	return status;
}

void
mx_ether_start_common(mx_instance_state_t *is, int mtu, int max_small_rx, 
		      int max_big_rx)
{
	MCP_SETVAL(is, ethernet_mtu, mtu);
	MCP_SETVAL(is, ethernet_smallbuf, max_small_rx);
}

/* it is expected that a driver would call this after disabling
   ethernet and freeing its recv buffers */
void
mx_ether_close_common(mx_instance_state_t *is)
{
	mx_spin_lock_destroy(&is->ether->spinlock);
	mx_sync_destroy(&is->ether->down_sync);
	myri_ether_free_common(is);
	is->ether_is_open = 0;
}

void
mx_ether_set_mac_address_common(struct mx_ether *sc, uint8_t *addr)
{
	static int warned = 0;

	if (!warned) {
		MX_WARN(("Setting ether mac address not yet supported\n"));
		warned++;
	}
}

int
mx_ether_set_promisc_common(struct mx_ether *eth, int promisc)
{
	uint32_t ctrl, dont_care;
	
	ctrl = promisc ? MCP_CMD_ENABLE_PROMISC : MCP_CMD_DISABLE_PROMISC;
	return mx_mcp_command(eth->is, ctrl, 0, promisc, 0, &dont_care);
}

int
mx_ether_set_pause_common(struct mx_ether *eth, int pause)
{
	uint32_t ctrl, dont_care;
	
	ctrl = pause ? MCP_CMD_ENABLE_FLOW_CONTROL :
		MCP_CMD_DISABLE_FLOW_CONTROL;
	return mx_mcp_command(eth->is, ctrl, 0, pause, 0, &dont_care);
}


#if MYRI_ENABLE_PTP
void
myri_ptp_store_rx_timestamp(struct mx_instance_state *is, uint8_t *pkt, uint64_t timestamp_ns)
{
	struct myri_eth_header *e_hdr = (struct myri_eth_header *) (pkt + MCP_ETHER_PAD);
	struct myri_vlan_header *v_hdr = NULL;
	struct myri_ip_header *ip_hdr = NULL;
	struct myri_udp_header *udp_hdr = NULL;
	struct myri_ptp_header *ptp_hdr = NULL;
	uint32_t len = 0;
	myri_ptp_store_rx_msg_t x;
	void *msg = NULL;
	uint8_t *tmp = NULL;

	x.raw = 0;

	if (unlikely(e_hdr->eh_type == htons(MYRI_ETHTYPE_VLAN))) {

		v_hdr = (struct myri_vlan_header *) (e_hdr + 1);

		if (unlikely(v_hdr->vh_proto == htons(MYRI_ETHTYPE_PTP))) {
			uint16_t ptp_len = ntohs(ptp_hdr->messageLength);

			ptp_hdr = (struct myri_ptp_header *) (v_hdr + 1);
			len = MYRI_ETH_HLEN + ptp_len;
			tmp = mx_kmalloc(len, MX_MZERO);
			if (!tmp)
				goto out;
			memcpy(tmp, e_hdr, MYRI_ETH_HLEN);
			memcpy(tmp + MYRI_ETH_HLEN, ptp_hdr, ptp_len);
			msg = tmp;
			x.raw = 1;
		} else if (likely(v_hdr->vh_proto == htons(MYRI_ETHTYPE_IP))) {
			ip_hdr = (struct myri_ip_header *) (v_hdr + 1);
		}
	}

	if (likely(!msg && !ip_hdr && !v_hdr && e_hdr->eh_type == htons(MYRI_ETHTYPE_IP)))
		ip_hdr = (struct myri_ip_header *) (e_hdr + 1);

	if (likely(ip_hdr != NULL)) {
		if (likely(ip_hdr->ih_proto != IPPROTO_UDP))
			goto out;

		udp_hdr = (struct myri_udp_header *) (ip_hdr + 1);
		if (likely(!(udp_hdr->uh_dport == htons(MYRI_PTP_EVENT_PORT) ||
			     udp_hdr->uh_dport == htons(MYRI_PTP_GENERAL_PORT))))
			goto out;

		ptp_hdr = (struct myri_ptp_header *) (udp_hdr + 1);
		msg = ptp_hdr;
		len = ntohs(ptp_hdr->messageLength);
	}

	if (unlikely(!msg && e_hdr->eh_type == htons(MYRI_ETHTYPE_PTP))) {
		ptp_hdr = (struct myri_ptp_header *) (e_hdr + 1);
		msg = e_hdr;
		len = MYRI_ETH_HLEN + ntohs(ptp_hdr->messageLength);
		x.raw = 1;
	}

	if (!msg)
		goto out;

	x.timestamp_ns = timestamp_ns;
	x.msg_pointer = (uint64_t)((uaddr_t)msg);
	x.msg_len = len;
	x.board = is->id;

	myri_ptp_store_rx_msg(is, &x, 1);
	if (unlikely(tmp != NULL))
		mx_kfree(tmp);
out:
	return;
}
#endif



/*
  This file uses MX driver indentation.

  Local Variables:
  c-file-style:"linux"
  tab-width:8
  End:
*/
