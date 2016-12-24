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
 * Copyright 2003 - 2009 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#include "mx_arch.h"
#include "mx_misc.h"
#include "mx_instance.h"
#include "mx_malloc.h"
#include "mx_ether_common.h"
#include "mal_stbar.h"
#include "mx_pio.h"

int mx_ether_fill_thresh = NUM_RX / 2;
int mx_ether_rx_alloc_size;
int mx_ether_lro_max_pkts = 64;
int mx_ether_napi_weight = 128;

/* compatibility cruft for multiple linux versions */

static struct net_device *
mx_netdev_alloc(char *s, int *err, int num)
{
	struct mx_ether *eth;
	struct net_device *dev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	char name[IFNAMSIZ];

	eth = mx_kmalloc(sizeof(*eth), MX_MZERO|MX_NOWAIT);
	if (!eth) {
		*err = -ENOMEM;
		return NULL;
	}
	if (myri_ether_ifname_legacy)
		sprintf(name, s, num);
	else
		strcpy(name, s);
	dev = alloc_netdev(sizeof(eth), name, ether_setup);
	if (!dev) {
		mx_kfree(eth);
		*err = -ENOMEM;
                return (dev);
        }
	
	*(struct mx_ether **)(netdev_priv(dev)) = eth;
#else
	eth = mx_kmalloc(sizeof(*eth), MX_MZERO|MX_NOWAIT);
	if (!eth) {
		*err = -ENOMEM;
		return NULL;
	}
	dev = dev_alloc(s,err);
	if (!dev) {
		mx_kfree(eth);
		*err = -ENOMEM;
		return (dev);
	}
	dev->priv = eth;
#endif
	*err = 0;
	return (dev);
}

static void
mx_netdev_free(struct net_device *d)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	mx_kfree(mx_netdev_priv(d));
	free_netdev(d);
#else
	mx_kfree(d->priv);
	kfree(d);
#endif
}

/* This is the max number of skb frags we expect we are likely to
   encounter from a well behaved sender.  Beyond that, we may
   linearize an skb so as to make it fit into the available space in
   the send ring */

#define MX_MAX_SKB_FRAGS (MX_ATOP(MX_PAGE_ALIGN(MX_MAX_ETHER_MTU - 14))  + 2)

/*
 * Encapsulate an skbuf for DMA.  This is tricky since we support
 * frags, and because iommu's require us to unmap memory.
 *
 * In the ring, the following hints are used by the tx completion
 * code:
 * bus INVALID, skb null: head frag
 * skb null: part of a frag
 * skb non-null: skb
 */

static inline int
mx_encap(struct mx_ether *eth, struct sk_buff *skb)
{
	dma_addr_t bus, frag_bus;
	struct skb_frag_struct *frag;
	mcp_kreq_ether_send_t req_list[MCP_ETHER_MAX_SEND_FRAG + 1];
	mcp_kreq_ether_send_t *req;
	int len;
	int head_frag_num;
	int extra_frag;
	
	int i, idx, avail, frag_cnt, f, err;

	/* leave one slot to keep ring from wrapping */
	avail = NUM_TX - 1 - (eth->tx.req - eth->tx.done);
	frag_cnt = skb_shinfo(skb)->nr_frags;

	if (avail < 1 + 4)
		return -ENOBUFS;

	/* if the 1 + 1 + frag_cnt is greater than available free
	   request slots, then  we will run out of space in
	   the ring.  This should never happen, as the
	   queue should have been stopped previously.
	*/

	extra_frag = MCP_TX_BOUNDARY == 2048 ? 2 : 0;
#if PAGE_SIZE > MCP_TX_BOUNDARY
	/* for this frag accounting, only look if the frag crosses a tx-boundary once,
	   if it crosses twice, it is taken into account in the constant "4" or "6" which
	   take into account areas longer than MCP_TX_BOUNDARY, whether they are in the head
	   or the frags (they can be no more than 2 of them for 4k, 4 of them for 2k, 
	   the additional 2 is for the head and tail of the main part
	*/
	for (f = 0; f < frag_cnt; f++) {
		frag = skb_shinfo(skb)->frags + f;
		if ((frag->page_offset & (MCP_TX_BOUNDARY - 1)) + frag->size > MCP_TX_BOUNDARY)
			extra_frag += 1;
	}
#endif
	if ((avail < (1 + 4 + frag_cnt))  ||
	    (4 + frag_cnt + extra_frag) >= MCP_ETHER_MAX_SEND_FRAG) {
		if (mx_skb_linearize(skb)) {
			return -ENOBUFS;
		}
		frag_cnt = 0;
	}

	i = eth->tx.req;
	req = req_list;
	pci_unmap_addr_set(&eth->tx.info[i & (NUM_TX - 1)], bus, INVALID_DMA_ADDR);
	i++;
	
	/* Give the MCP the dest addr (already in network byte order) */
	req->head.dest_high16 = *(uint16_t *)&skb->data[0];
	req->head.dest_low32 = *(uint32_t *)&skb->data[2];
	req->head.flags = MCP_ETHER_FLAGS_VALID | MCP_ETHER_FLAGS_HEAD;
	mx_ether_gw_tx(eth->is, req, skb->data);

	/* Setup checksum offloading, if needed */
	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		uint16_t pseudo_hdr_offset;
		uint16_t cksum_offset;
		
		pseudo_hdr_offset = skb_transport_offset(skb) + skb->csum_offset;
		cksum_offset =  skb_transport_offset(skb);
		/* FIXME: if first frag cross 4k boundary before csum_off or pseudo_hdr */
		if (pseudo_hdr_offset < MCP_ETHER_MAX_PSEUDO_OFF
		    && pskb_may_pull(skb, pseudo_hdr_offset + 2)
		    && cksum_offset < 254
		    && pskb_may_pull(skb, cksum_offset + 2)) {
			req->head.pseudo_hdr_offset = htons(pseudo_hdr_offset);
			req->head.cksum_offset = htons(cksum_offset);
			req->head.flags |= MCP_ETHER_FLAGS_CKSUM;
		} else {
			/* do it in software */
			uint32_t csum;

			req->head.pseudo_hdr_offset = 0;
			req->head.cksum_offset = 0;
			csum = skb_checksum(skb, cksum_offset, 
					    skb->len - cksum_offset, 0);
			csum = csum_fold(csum);
			MX_PRINT_ONCE(("Warning: sum field offset (%d,%d) max=%d\n"
				       "\t head_len=%d, len=%d, frag_list=%p\n",
				       pseudo_hdr_offset, cksum_offset, MCP_ETHER_MAX_PSEUDO_OFF,
				       skb_headlen(skb), skb->len, skb_shinfo(skb)->frag_list));
			if (pskb_may_pull(skb, pseudo_hdr_offset + 2)) {
				*(uint16_t*)(skb_transport_header(skb) + skb->csum_offset) = csum;
			}
		}
	} else {
		req->head.pseudo_hdr_offset = 0;
		req->head.cksum_offset = 0;
	}
	req++;

	/* map the skbuf */
	idx = i & (NUM_TX - 1);
	bus = pci_map_single(eth->is->arch.pci_dev,
			     skb->data, skb_headlen(skb),
			     PCI_DMA_TODEVICE);
	if (bus == INVALID_DMA_ADDR)
		return -ENXIO;

	eth->tx.info[idx].u.skb = skb;
	len = skb_headlen(skb);

	frag_bus = bus;
	head_frag_num = 0;
	do {
		unsigned frag_len = len;

		head_frag_num += 1;
		mal_assert(head_frag_num <= 4);
		mal_assert(head_frag_num ==1 || (bus & 0xfff) == 0);
		/* don't cross 4K barrier */
		if ((bus & 0xfff) + len > 4096)
			frag_len = 4096 - (bus & 0xfff);

		idx = i & (NUM_TX - 1);

		pci_unmap_addr_set(&eth->tx.info[idx], bus, frag_bus);
		pci_unmap_len_set(&eth->tx.info[idx], len, len);
		/* store DMA address and len */
		req->frag.addr_low = htonl(MX_LOWPART_TO_U32(bus));
		req->frag.addr_high = htonl(MX_HIGHPART_TO_U32(bus));
		req->frag.length = htonl(frag_len);
		/* This is used only for debugging */
		req->frag.flags = MCP_ETHER_FLAGS_VALID;

		i++;
		req++;
		frag_bus = INVALID_DMA_ADDR; /* first iteration record whole area */
		len -= frag_len;
		bus += frag_len;
	} while (len > 0);

	if (frag_cnt) {
		/* now attempt to map the frags, if we have any */
		for (f = 0; f < frag_cnt; f++, i++) {
			idx = i & (NUM_TX - 1);
			frag = &skb_shinfo(skb)->frags[f];
			bus = pci_map_page(eth->is->arch.pci_dev,
					   frag->page,
					   frag->page_offset,
					   frag->size,
					   PCI_DMA_TODEVICE);
			if (bus == INVALID_DMA_ADDR) {
				err = -ENXIO;
				goto abort;
			}
			pci_unmap_addr_set(&eth->tx.info[idx], bus, bus);
			pci_unmap_len_set(&eth->tx.info[idx], len, frag->size);
			req->frag.addr_low = htonl(MX_LOWPART_TO_U32(bus));
			req->frag.addr_high = htonl(MX_HIGHPART_TO_U32(bus));
			req->frag.length = htonl(frag->size);
			/* This is used only for debugging */
			req->frag.flags = MCP_ETHER_FLAGS_VALID;
			req++;
		}
	}


	/* account for slots used by head + main skbuf + any frags*/
	avail -= (1 + head_frag_num + frag_cnt); 

	/* This check must go before the device can send
	   the frame so that we are always assured of getting
	   at least one tx complete irq after the  queue
	   has been stopped. */

	if (avail < MX_MAX_SKB_FRAGS + 1 + 4)
		netif_stop_queue(eth->arch.dev);

	/* terminate the request chain */
	req[-1].frag.flags |= MCP_ETHER_FLAGS_LAST;

	/* Tell the lanai about it */
	mx_ether_submit_tx_req(eth, req_list, 1 + head_frag_num + frag_cnt);
	mx_mmiowb();
	return 0;

	
abort:
	for (i--; i != eth->tx.req; i--) {
		idx = i & (NUM_TX - 1);
		if (eth->tx.info[idx].u.skb != 0 ) {
			eth->tx.info[idx].u.skb = 0;
			pci_unmap_single(eth->is->arch.pci_dev,
					 pci_unmap_addr(&eth->tx.info[idx], bus),
					 pci_unmap_len(&eth->tx.info[idx],  len),
					 PCI_DMA_TODEVICE);
		} else if (pci_unmap_addr(&eth->tx.info[idx], bus) != INVALID_DMA_ADDR) {
			pci_unmap_page(eth->is->arch.pci_dev,
				       pci_unmap_addr(&eth->tx.info[idx], bus),
				       pci_unmap_len(&eth->tx.info[idx],  len),
				       PCI_DMA_TODEVICE);
		}
	}
	return err;
}

static int
mx_ether_xmit(struct sk_buff *skb, struct net_device *dev)
{
	int err;
	
#if MX_10G_ENABLED
	if (skb->len < ETH_ZLEN) {
		struct mx_ether *eth = mx_netdev_priv(dev);
		if (mx_skb_padto(skb, ETH_ZLEN) != 0) {
			eth->arch.stats.tx_dropped += 1;
			return 0;
		}
		skb->len = ETH_ZLEN;
	}
#endif
	
	err = mx_encap(mx_netdev_priv(dev), skb);
	
	if (err) {
		return 1;
	}
	dev->trans_start = jiffies;
	return 0;
}


void
mx_ether_tx_done(mx_instance_state_t *is, uint32_t mcp_index)
{
	struct mx_ether *eth;
	struct sk_buff *skb;
	int idx;

	eth = is->ether;

	while (eth->tx.done != mcp_index) {
		unsigned unmap_len;
		dma_addr_t bus = INVALID_DMA_ADDR; /* stupid gcc */

		idx = eth->tx.done & (NUM_TX - 1);
		bus = pci_unmap_addr(&eth->tx.info[idx], bus);
		unmap_len = pci_unmap_len(&eth->tx.info[idx], len);
		skb = eth->tx.info[idx].u.skb;
		eth->tx.info[idx].u.skb = 0;
		eth->tx.done++;


		if (skb) {
			/* Unmap an skbuf */
			pci_unmap_single(eth->is->arch.pci_dev,
					 bus,
					 unmap_len,
					 PCI_DMA_TODEVICE);
			/* Update our stats */
			eth->arch.stats.tx_bytes += skb->len;
			eth->arch.stats.tx_packets++;
			/* Free the skb */
			dev_kfree_skb_irq(skb);
		} else if (bus != INVALID_DMA_ADDR) {
			/* Unmap a fragment */
			pci_unmap_page(eth->is->arch.pci_dev,
				       bus,
				       unmap_len,
				       PCI_DMA_TODEVICE);
		}
	}
	
	/* start the queue if we've stopped it */
	if (netif_queue_stopped(eth->arch.dev) 
	    && eth->tx.req - eth->tx.done < (NUM_TX >> 2)
	    && eth->running == MX_ETH_RUNNING)
		netif_wake_queue(eth->arch.dev);
}

static void
mx_ether_alloc_rx_pages(mx_instance_state_t *is, mx_ether_rx_buf_t *rx,
			int bytes, int watchdog)
{
	struct page *page;
	int idx;

	if (unlikely(rx->watchdog_needed && !watchdog))
		return;

	bytes += MCP_ETHER_PAD;

	/* try to refill entire ring */
	while ((rx->fill_cnt - rx->cnt) != NUM_RX) {
		idx = rx->fill_cnt & (NUM_RX - 1);
		if (rx->page_offset + bytes <= mx_ether_rx_alloc_size) {
			/* we can use part of previous page */
			get_page(rx->page);
		} else {
			/* we need a new page */
			page = alloc_pages(GFP_ATOMIC|MX_GFP_COMP|MX_GFP_NOWARN,
					   myri_ether_rx_alloc_order);
			if (unlikely(page == NULL)) {
				if (rx->fill_cnt - rx->cnt < 16)
					rx->watchdog_needed = 1;
				return;
			}
			rx->page = page;
			rx->page_offset = 0;
			rx->bus = pci_map_page(is->arch.pci_dev, page, 0,
					       mx_ether_rx_alloc_size,
					       PCI_DMA_FROMDEVICE);
		}

		rx->info[idx].u.pg_info.page = rx->page;
		rx->info[idx].u.pg_info.page_offset = rx->page_offset;

		/* note that this is the address of the start of the
		 * page */

		pci_unmap_addr_set(&rx->info[idx], bus, rx->bus);
		rx->shadow[idx].low = htonl(MX_LOWPART_TO_U32(rx->bus) + rx->page_offset);
		rx->shadow[idx].high = htonl(MX_HIGHPART_TO_U32(rx->bus));

		/* start next packet on a cacheline boundary */
		rx->page_offset += SKB_DATA_ALIGN(bytes);

		/* don't cross a 4KB boundary */
		if ((rx->page_offset >> 12) !=
		    ((rx->page_offset + bytes - 1) >> 12))
			rx->page_offset = (rx->page_offset + 4096) & ~4095;
		rx->fill_cnt++;

		/* copy 4 descriptors to the firmware at a time */
		if ((idx & 3) == 3) {
			mx_pio_memcpy(&rx->ring[idx - 3], &rx->shadow[idx - 3],
				      4 * sizeof (*rx->ring), 0);
			MAL_STBAR();
			MX_PIO_WRITE(rx->lanai_cnt, htonl(rx->fill_cnt));
		}
	}
}

/*
 * Allocate an skb for receive.  We must ensure that
 * big buffers are aligned on a 4KB boundary
 *
 */

static inline int
mx_get_buf(struct net_device *dev, struct mx_ether *eth, 
	   mx_ether_rx_buf_t *rx, int idx, int bytes, int gfp_mask)
{
	struct sk_buff *skb;
	dma_addr_t bus;
	uintptr_t data, roundup, pad;
	int len, retval = 0;

	/* 
	   The pad is used to deterimine the roundup, so it
	   must be a non-zero power of 2.  This means we must
	   allocate an extra 16 bytes for smalls */
	pad = 16;
	if (bytes > 4096)
		pad = 4096;

	len = SKB_ROUNDUP(bytes + pad + MCP_ETHER_PAD);
	skb = alloc_skb(len + 16, gfp_mask);
	if (!skb) {
		rx->alloc_fail++;
		retval = -ENOBUFS;
		goto done;
	}
	/* mimic dev_alloc_skb() and add at least bytes of headroom.
	   we need to add more if this is a big buffer, and we're
	   aligning to the start of a 4KB chunk
	*/
	data = ((uintptr_t)(skb->data) + 16 + pad);
	roundup = (data & ~(pad - 1)) - ((uintptr_t)(skb->data));
	skb_reserve(skb, roundup);

	/* re-set len so that it only covers the area we
	   need mapped for DMA */
	len = bytes + MCP_ETHER_PAD;
	
	bus = pci_map_single(eth->is->arch.pci_dev,
			     skb->data, len, PCI_DMA_FROMDEVICE);
	if (bus == INVALID_DMA_ADDR) {
		dev_kfree_skb_any(skb);
		retval = -ENXIO;
		goto done;
	}

	/* make sure it does not cross a 4GB boundary */
	mal_assert((uint32_t)(len + MX_LOWPART_TO_U32(bus)) > 
		  (uint32_t)(MX_LOWPART_TO_U32(bus)));

	rx->info[idx].u.skb = skb;
	pci_unmap_addr_set(&rx->info[idx], bus, bus);
	pci_unmap_len_set(&rx->info[idx], len, len);
	rx->shadow[idx].low = htonl(MX_LOWPART_TO_U32(bus));
	rx->shadow[idx].high = htonl(MX_HIGHPART_TO_U32(bus));
       
done:
	/* copy 4 descriptors to the mcp at a time */
	if ((idx & 3) == 3) {
		/* 4 descriptors == 32 bytes for Z fast-writes */
		mx_pio_memcpy(&rx->ring[idx - 3], &rx->shadow[idx - 3],
			    4 * sizeof (*rx->ring), 0);
		MAL_STBAR();
		MX_PIO_WRITE(rx->lanai_cnt, htonl(rx->cnt));
	}
        return retval;
}


int
mx_get_buf_big(struct mx_ether *eth, int idx, int gfp_mask)
{
	struct mx_ether_buffer_info *info;
	dma_addr_t bus;
	struct page *page;
	mx_ether_rx_buf_t *rx = &eth->rx_big;
	int retval = 0;

	page = alloc_page(gfp_mask);
	if (!page) {
		retval = -ENOMEM;
		goto done;
	}
	bus = pci_map_page(eth->is->arch.pci_dev,
			   page, 0, PAGE_SIZE,
			   PCI_DMA_FROMDEVICE);
	if (bus == INVALID_DMA_ADDR) {
		put_page(page);
		retval = -ENOMEM;
		goto done;
	}
	info = eth->rx_big.info + idx;
	info->u.pg_info.page = page;
	pci_unmap_addr_set(info, bus, bus);
	pci_unmap_len_set(info, len, PAGE_SIZE);
	rx->shadow[idx].low = htonl(MX_LOWPART_TO_U32(bus));
	rx->shadow[idx].high = htonl(MX_HIGHPART_TO_U32(bus));

done:
	return retval;
}                         

#ifdef HAVE_LRO
void
mx_ether_lro_flush(mx_instance_state_t *is)
{
	lro_flush_all(&is->ether->arch.lro_mgr);
}

static int
mx_ether_get_frag_header(struct skb_frag_struct *frag, void **mac_hdr,
			  void **ip_hdr, void **tcpudp_hdr,
			  u64 * hdr_flags, void *priv)
{
	struct ethhdr *eh;
	struct vlan_ethhdr *veh;
	struct iphdr *iph;
	u8 *va = page_address(frag->page) + frag->page_offset;

	unsigned long ll_hlen;
	__wsum csum = (__wsum) (unsigned long)priv;

	/* find the mac header, aborting if not IPv4 */

	eh = (struct ethhdr *)va;
	*mac_hdr = eh;

	ll_hlen = ETH_HLEN;
	if (eh->h_proto != htons(ETH_P_IP)) {
		if (eh->h_proto == htons(ETH_P_8021Q)) {
			veh = (struct vlan_ethhdr *)va;
			if (veh->h_vlan_encapsulated_proto != htons(ETH_P_IP))
				return -1;

			ll_hlen += VLAN_HLEN;

			/*
			 *  HW checksum starts ETH_HLEN bytes into
			 *  frame, so we must subtract off the VLAN
			 *  header's checksum before csum can be used
			 */
			csum = csum_sub(csum, csum_partial(va + ETH_HLEN,
							   VLAN_HLEN, 0));
		} else {
			return -1;
		}
	}
	*hdr_flags = LRO_IPV4;

	iph = (struct iphdr *)(va + ll_hlen);
	*ip_hdr = iph;
	if (iph->protocol != IPPROTO_TCP)
		return -1;

	*hdr_flags |= LRO_TCP;
	*tcpudp_hdr = (u8 *) (*ip_hdr) + (iph->ihl << 2);

	/* verify the IP checksum */
	if (unlikely(ip_fast_csum((u8 *) iph, iph->ihl)))
		return -1;

	/* verify the  checksum */
	if (unlikely(csum_tcpudp_magic(iph->saddr, iph->daddr,
				       ntohs(iph->tot_len) - (iph->ihl << 2),
				       IPPROTO_TCP, csum)))
		return -1;

	return 0;
}

static int
mx_ether_get_skb_header(struct sk_buff *skb,
                        void **ip_hdr,  void **tcpudp_hdr,
                        u64 *hdr_flags, void *priv)
{
	struct ethhdr *eh;
	struct vlan_ethhdr *veh;
	struct iphdr *iph;
	u8 *va = skb->data - ETH_HLEN;
	unsigned long ll_hlen;
	__wsum csum = (__wsum) (unsigned long)priv;

	/* find the mac header, aborting if not IPv4 */

	eh = (struct ethhdr *)va;
	ll_hlen = ETH_HLEN;
	if (eh->h_proto != htons(ETH_P_IP)) {
		if (eh->h_proto == htons(ETH_P_8021Q)) {
			veh = (struct vlan_ethhdr *)va;
			if (veh->h_vlan_encapsulated_proto != htons(ETH_P_IP))
				return -1;

			ll_hlen += VLAN_HLEN;

			/*
			 *  HW checksum starts ETH_HLEN bytes into
			 *  frame, so we must subtract off the VLAN
			 *  header's checksum before csum can be used
			 */
			csum = csum_sub(csum, csum_partial(va + ETH_HLEN,
							   VLAN_HLEN, 0));
		} else {
			return -1;
		}
	}
	*hdr_flags = LRO_IPV4;

	iph = (struct iphdr *)(va + ll_hlen);
	*ip_hdr = iph;
	if (iph->protocol != IPPROTO_TCP)
		return -1;
	*hdr_flags |= LRO_TCP;
	*tcpudp_hdr = (u8 *) (*ip_hdr) + (iph->ihl << 2);

	/* verify the IP checksum */
	if (unlikely(ip_fast_csum((u8 *) iph, iph->ihl)))
		return -1;

	/* verify the  checksum */
	if (unlikely(csum_tcpudp_magic(iph->saddr, iph->daddr,
				       ntohs(iph->tot_len) - (iph->ihl << 2),
				       IPPROTO_TCP, csum)))
		return -1;

	return 0;
}
#endif

static inline void
mx_ether_vlan_ip_csum(struct sk_buff *skb, u32 hw_csum)
{
	struct vlan_hdr *vh = (struct vlan_hdr *) (skb->data);

	if ((skb->protocol == htons(ETH_P_8021Q)) &&
	    (vh->h_vlan_encapsulated_proto == htons(ETH_P_IP) ||
	     vh->h_vlan_encapsulated_proto == htons(ETH_P_IPV6))) {
		skb->csum = hw_csum;
		skb->ip_summed = CHECKSUM_COMPLETE;
		if (myri_ether_vlan_csum_fixup)
			skb->csum =
				csum_sub(skb->csum,
					 csum_partial(skb->data,
						      VLAN_HLEN, 0));
	}
}


static inline void 
mx_ether_rx_done_skb(struct mx_ether *eth, struct net_device *dev,
		     mx_ether_rx_buf_t *rx, int bytes, int len, 
		     int csum, int flags, uint64_t timestamp)
{
	dma_addr_t bus;
	struct sk_buff *skb;
	int idx, unmap_len;

	idx = rx->cnt & (NUM_RX - 1);
	rx->cnt++;
	
	/* save a pointer to the received skb */
	skb = rx->info[idx].u.skb;
	bus = pci_unmap_addr(&rx->info[idx], bus);
	unmap_len = pci_unmap_len(&rx->info[idx], len);

	/* try to replace the received skb */
	if (mx_get_buf(dev, eth, rx, idx, bytes, GFP_ATOMIC)) {
		/* drop the frame -- the old skbuf is re-cycled */
		eth->arch.stats.rx_dropped += 1;
		return;
	}

	/* unmap the recvd skb */
	pci_unmap_single(eth->is->arch.pci_dev,
			 bus, unmap_len,
			 PCI_DMA_FROMDEVICE);

	/* set the length of the frame */
	/* mcp implicitly skips 1st bytes so that packet is properly
	 * aligned */
	skb_put(skb, len + MCP_ETHER_PAD);
	mx_ether_gw_rx(eth->is, skb->data);
	myri_ptp_store_rx_timestamp(eth->is, skb->data, timestamp);
	skb_pull(skb, MCP_ETHER_PAD);


	skb->protocol = eth_type_trans(skb, dev);
	skb->dev = dev;
	if (eth->csum_flag & MCP_ETHER_FLAGS_CKSUM) {
		if ((skb->protocol == htons(ETH_P_IP)) ||
		    (skb->protocol == htons(ETH_P_IPV6))) {
			skb->csum = csum;
			skb->ip_summed = CHECKSUM_COMPLETE;
		} else {
			mx_ether_vlan_ip_csum(skb, csum);
		}
#ifdef HAVE_LRO
		if (myri_ether_lro) {
			lro_receive_skb(&eth->arch.lro_mgr, skb,
					(void *)(unsigned long)csum);
			goto done;
		}
#endif
	}
	mx_rx_skb(skb);
	dev->last_rx = jiffies;
#ifdef HAVE_LRO
done:
#endif
	eth->arch.stats.rx_packets += 1;
	eth->arch.stats.rx_bytes += len;
}


#define MX_MAX_FRAGS_PER_FRAME (MX_MAX_ETHER_MTU / 4096 + 1)

static inline void
mx_ether_unmap_rx_page(struct pci_dev *pdev,
		       struct mx_ether_buffer_info *info, int bytes)
{
	/* unmap the recvd page if we're the only or last user of it */
	if (bytes >= mx_ether_rx_alloc_size/2 || 
	    (info->u.pg_info.page_offset + 2 * bytes) > mx_ether_rx_alloc_size) {
		pci_unmap_page(pdev, 
			       (pci_unmap_addr(info, bus)
				& ~(mx_ether_rx_alloc_size - 1)),
			       mx_ether_rx_alloc_size,
			       PCI_DMA_FROMDEVICE);
	}
}

static void 
mx_ether_rx_done_page(struct mx_ether *eth, struct net_device *dev,
		      mx_ether_rx_buf_t *rx, int bytes, int count, int len, 
		      int csum, int flags, uint64_t timestamp)
{
	struct skb_frag_struct rx_frags[MX_MAX_FRAGS_PER_FRAME];
	struct sk_buff *skb;
	struct skb_frag_struct *skb_frags;
	int seg, idx, hlen, remainder;
	u8 *va;

	mal_assert(count <= MX_MAX_FRAGS_PER_FRAME);
	len += MCP_ETHER_PAD; /* the mcp only gives the "payload" */
	idx = rx->cnt & (NUM_RX - 1);
	va = page_address(rx->info[idx].u.pg_info.page) + 
		rx->info[idx].u.pg_info.page_offset;
	prefetch(va);
	myri_ptp_store_rx_timestamp(eth->is, va, timestamp);

	/* Fill skb_frag_struct(s) with data from our receive */
	for (seg = 0, remainder = len; remainder > 0; seg++) {
		mx_ether_unmap_rx_page(eth->is->arch.pci_dev,
				       &rx->info[idx], bytes);
		rx_frags[seg].page = rx->info[idx].u.pg_info.page;
		rx_frags[seg].page_offset =
			rx->info[idx].u.pg_info.page_offset;

		if (remainder < mx_ether_rx_alloc_size)
			rx_frags[seg].size = remainder;
		else
			rx_frags[seg].size = mx_ether_rx_alloc_size;
		rx->cnt++;
		idx = rx->cnt & (NUM_RX - 1);
		remainder -= mx_ether_rx_alloc_size;
	}

	mx_ether_gw_rx(eth->is, va);

#ifdef HAVE_LRO
	if ((eth->csum_flag & MCP_ETHER_FLAGS_CKSUM) && myri_ether_lro) {
		rx_frags[0].page_offset +=  MCP_ETHER_PAD;
		rx_frags[0].size -=  MCP_ETHER_PAD;
		len -=  MCP_ETHER_PAD;
		lro_receive_frags(&eth->arch.lro_mgr, rx_frags,
				  len, len, (void *)(unsigned long)csum, csum);
		goto done;
	}
#endif

	/* allocate skb late to avoid allocation overheads for LRO */
	skb = alloc_skb(64, GFP_ATOMIC);
	if (unlikely(skb == NULL)) {
		eth->arch.stats.rx_dropped++;
		do {
			seg--;
			put_page(rx_frags[seg].page);
		} while (seg != 0);		
		return;
	}

	hlen = 64 > len ? len: 64;

	skb->len = skb->data_len = len;
	skb->truesize = sizeof (struct sk_buff) + len;

	skb_frags = skb_shinfo(skb)->frags;
	seg = 0;
	while (len > 0) {
		memcpy(skb_frags, &rx_frags[seg], sizeof (*skb_frags));
		len -= rx_frags->size;
		skb_frags++;
		seg++;
		skb_shinfo(skb)->nr_frags++;
	}

	/* pskb_may_pull is not available in irq context, but
	   skb_pull() (for ether_pad and eth_type_trans()) requires
	   the beginning of the packet in skb_headlen(), move it
	   manually */
	memcpy(skb->data, va, hlen);
	skb->tail += hlen;
	skb->data_len -= hlen; 

	skb_shinfo(skb)->frags[0].page_offset += hlen;
	skb_shinfo(skb)->frags[0].size -= hlen;
	skb_pull(skb, MCP_ETHER_PAD);
	if (skb_shinfo(skb)->frags[0].size <= 0) {
		put_page(skb_shinfo(skb)->frags[0].page);
		skb_shinfo(skb)->nr_frags = 0;
	}
	skb->protocol = eth_type_trans(skb, dev);
	if (eth->csum_flag & MCP_ETHER_FLAGS_CKSUM) {
		if ((skb->protocol == htons(ETH_P_IP)) ||
		    (skb->protocol == htons(ETH_P_IPV6))) {
			skb->csum = csum;
			skb->ip_summed = CHECKSUM_COMPLETE;
		} else {
			mx_ether_vlan_ip_csum(skb, csum);
		}
	}
	skb->dev = dev;
	len = skb->len;
	mx_rx_skb(skb);
	dev->last_rx = jiffies;
#ifdef HAVE_LRO
done:
#endif
	eth->arch.stats.rx_packets++;
	eth->arch.stats.rx_bytes += len;
}

void 
mx_ether_rx_done_small(mx_instance_state_t *is, mcp_ether_rx_desc_t *desc)
{
	struct mx_ether *eth;
	struct net_device *dev;
	mx_ether_rx_buf_t *rx;
	int bytes;
	int count = desc->buf_cnt;
	int len = htons(desc->len);
	int csum = htons(desc->cksum);
	int flags = desc->flags;
	uint64_t timestamp;

#if MYRI_ENABLE_PTP
	timestamp = myri_ptp_rx_nticks_to_nsecs(is, desc);
#else
	timestamp = 0ULL; /* -Wunused on non-linux */
#endif

	mal_assert(count == 1);
	eth = is->ether;
	dev = eth->arch.dev;
	rx = &eth->rx_small;
	bytes = eth->arch.small_bytes;
	if (!myri_ether_rx_frags) {
		mx_ether_rx_done_skb(eth, dev, rx, bytes, len,
				     ntohs((uint16_t)csum), flags, timestamp);
	} else {
		mx_ether_rx_done_page(eth, dev, rx, bytes, count, len,
				      ntohs((uint16_t)csum), flags, timestamp);
		/* restock receive rings if needed */
		if (rx->fill_cnt - rx->cnt < mx_ether_fill_thresh)
			mx_ether_alloc_rx_pages(is, rx, bytes, 0);
	}
}

void 
mx_ether_rx_done_big(mx_instance_state_t *is, mcp_ether_rx_desc_t *desc)
{
	struct mx_ether *eth;
	struct net_device *dev;
	mx_ether_rx_buf_t *rx;
	int bytes;
	int count = desc->buf_cnt;
	int len = htons(desc->len);
	int csum = htons(desc->cksum);
	int flags = desc->flags;
	uint64_t timestamp;

#if MYRI_ENABLE_PTP
	timestamp = myri_ptp_rx_nticks_to_nsecs(is, desc);
#else
	timestamp = 0ULL; /* -Wunused on non-linux */
#endif

	eth = is->ether;
	dev = eth->arch.dev;
	rx = &eth->rx_big;
	bytes = dev->mtu + ETH_HLEN + VLAN_HLEN;
	if (!myri_ether_rx_frags) {
		mal_assert(count == 1);
		mx_ether_rx_done_skb(eth, dev, rx, bytes, len,
				     ntohs((uint16_t)csum), flags, timestamp);
	} else {
		mx_ether_rx_done_page(eth, dev, rx, bytes, count, len,
				      ntohs((uint16_t)csum), flags, timestamp);
		/* restock receive rings if needed */
		if (rx->fill_cnt - rx->cnt < mx_ether_fill_thresh)
			mx_ether_alloc_rx_pages(is, rx, bytes, 0);

	}
}

#ifdef HAVE_NEW_NAPI
static int
mx_ether_poll(struct napi_struct *napi, int budget)
{
	struct mx_ether_arch *arch = container_of(napi, struct mx_ether_arch, napi);
	struct mx_ether *eth;
	mx_instance_state_t *is;
	int work_done = 0;
	mcp_ether_rx_desc_t *desc;
	unsigned int cnt, page, offset;

	eth = container_of(arch, struct mx_ether, arch);
	is = eth->is;

	while (work_done < budget) {
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
		work_done++;
	}
out:
	if (work_done < budget) {
		/* re-enable interrupts */
#ifdef HAVE_LRO
		lro_flush_all(&is->ether->arch.lro_mgr);
#endif
		napi_complete(napi);
		MX_PIO_WRITE((uint32_t*)(is->lanai.sram + 0x6c0000), 
			     htonl(eth->rx_desc.cnt));
	}

	return (work_done);
}
#endif
static void
mx_ether_free_rx_pages(mx_instance_state_t *is, mx_ether_rx_buf_t *rx, int
		       bytes)
{
	int i, idx;

	for (i = rx->cnt; i < rx->fill_cnt; i++) {
		idx = i & (NUM_RX - 1);

		mx_ether_unmap_rx_page(is->arch.pci_dev, &rx->info[idx],
				       bytes);
		put_page(rx->info[idx].u.pg_info.page);
		rx->info[idx].u.pg_info.page = 0;
	}
}

static int
mx_ether_set_pause(struct net_device *dev, int pause)
{
	struct mx_ether *eth = mx_netdev_priv(dev);
	int status;

	status = mx_ether_set_pause_common(eth, pause);
	if (status) {
		MX_WARN(("%s: Failed to set flow control mode\n", dev->name));
		return status;
	}
	eth->pause = pause;
	return 0;
}

static int
mx_ether_close(struct net_device *dev)
{
	dma_addr_t bus;
	struct sk_buff *skb;
	struct mx_ether *eth;
	uint32_t dont_care;
	int i, unmap_len;

	eth = mx_netdev_priv(dev);

	/* if buffers not alloced, give up */
	if (!eth->rx_big.shadow)
		return -ENOTTY;

	/* if the device not running give up */
	if (eth->running != MX_ETH_RUNNING &&
	    eth->running != MX_ETH_OPEN_FAILED)
		return -ENOTTY;

	eth->running = MX_ETH_STOPPING;
	netif_tx_disable(dev);
	mx_mcp_command(eth->is, MCP_CMD_ETHERNET_DOWN,
		       0, 0, 0, &dont_care);
	mx_sleep(&eth->down_sync, MCP_COMMAND_TIMEOUT, MX_SLEEP_INTR);
	eth->running = MX_ETH_STOPPED;
#ifdef HAVE_NEW_NAPI
	napi_disable(&eth->arch.napi);
#endif

	/* free recvs */

	if (myri_ether_rx_frags) {
		mx_ether_free_rx_pages(eth->is, &eth->rx_small,
				       eth->arch.small_bytes);
		mx_ether_free_rx_pages(eth->is, &eth->rx_big,
				       dev->mtu + ETH_HLEN + VLAN_HLEN);
	}

	for (i = 0; i < NUM_RX; i++) {
		eth->rx_small.shadow[i].low = 0;
		eth->rx_small.shadow[i].high = 0;
		skb = eth->rx_small.info[i].u.skb;	
		bus = pci_unmap_addr(&eth->rx_small.info[i], bus);
		unmap_len = pci_unmap_len(&eth->rx_small.info[i], len);
		eth->rx_small.info[i].u.skb = 0;
		if (skb && !myri_ether_rx_frags) {
			pci_unmap_single(eth->is->arch.pci_dev,
					 bus, unmap_len,
					 PCI_DMA_FROMDEVICE);
			dev_kfree_skb(skb);
		}

		eth->rx_big.shadow[i].low = 0;
		eth->rx_big.shadow[i].high = 0;
		skb = eth->rx_big.info[i].u.skb;
		bus = pci_unmap_addr(&eth->rx_big.info[i], bus);
		unmap_len = pci_unmap_len(&eth->rx_big.info[i], len);
		eth->rx_big.info[i].u.skb = 0;
		if (skb && !myri_ether_rx_frags) {
			pci_unmap_single(eth->is->arch.pci_dev,
					 bus, unmap_len,
					 PCI_DMA_FROMDEVICE);
			dev_kfree_skb(skb);
		}
	}

	/* free transmits */

	while (eth->tx.done != eth->tx.req) {
		unsigned unmap_len;
		dma_addr_t bus = INVALID_DMA_ADDR; /* stupid gcc */

		i = eth->tx.done & (NUM_TX - 1);
		bus = pci_unmap_addr(&eth->tx.info[i], bus);
		unmap_len = pci_unmap_len(&eth->tx.info[i], len);
		skb = eth->tx.info[i].u.skb;
		eth->tx.done++;
		if (skb) {
			eth->tx.info[i].u.skb = 0;
			dev_kfree_skb(skb);
			eth->arch.stats.tx_dropped += 1;
			pci_unmap_single(eth->is->arch.pci_dev,
					 bus,
					 unmap_len,
					 PCI_DMA_TODEVICE);
		} else if (bus != INVALID_DMA_ADDR) {
			pci_unmap_page(eth->is->arch.pci_dev,
				       bus,
				       unmap_len,
				       PCI_DMA_TODEVICE);
		}
	}
	mx_ether_close_common(eth->is);
	return 0;
}

static int
mx_ether_open(struct net_device *dev)
{
	int error, i, mx_big_pow2, mx_big_thresh;
	uint32_t dont_care;
	struct mx_ether *eth;
#ifdef HAVE_LRO
	struct net_lro_mgr *lro_mgr;
#endif

	if (MX_DMA_DBG)
		return -ENOSYS; /* ether code does not update dma bitmap */
	eth = mx_netdev_priv(dev);

	if (eth->running != MX_ETH_STOPPED)
		return 0;

	/* decide what small buffer size to use.  For good TCP rx
	 * performance, it is important to not receive 1514 byte
	 * frames into jumbo buffers, as it confuses the socket buffer
	 * accounting code, leading to drops and erratic performance.
	 */

	if (dev->mtu <= ETH_DATA_LEN)
		/* enough for a TCP header */
		eth->arch.small_bytes = (128 > SMP_CACHE_BYTES) 
			? (128 - MCP_ETHER_PAD)
			: (SMP_CACHE_BYTES - MCP_ETHER_PAD); 
	else
		/* enough for a vlan encapsulated ETH_DATA_LEN frame.
		 *  We add 8 bytes of padding since the firmware does
		 *  some rounding
		 */
		eth->arch.small_bytes = MX_VLAN_ETH_FRAME_LEN + 8;

	if (myri_ether_rx_frags) {
		mx_big_thresh = mx_ether_rx_alloc_size;
	} else {
		/* Firmware needs the big buff size as a power of 2.  Lie and
		   tell him the buffer is larger, because we only use 1
		   buffer/pkt, and the mtu will prevent overruns */

		mx_big_pow2 = dev->mtu + MCP_ETHER_PAD + ETH_HLEN + VLAN_HLEN;
		while ((mx_big_pow2 & (mx_big_pow2 - 1)) != 0)
			mx_big_pow2++;
		mx_big_thresh = mx_big_pow2;
	}
	error = mx_ether_open_common(eth->is, dev->mtu + ETH_HLEN + VLAN_HLEN,
				     eth->arch.small_bytes + MCP_ETHER_PAD,
				     mx_big_thresh);

	if (error) {
		MX_WARN(("%s: mx_ether_open_common() failed, errno = %d\n",
			 dev->name, error));
		goto abort_with_nothing;
	}

	/* allocate recvs */
	if (myri_ether_rx_frags) {
		eth->rx_small.fill_cnt = 0;
		eth->rx_big.fill_cnt = 0;
		eth->rx_small.page_offset = mx_ether_rx_alloc_size;
		eth->rx_big.page_offset = mx_ether_rx_alloc_size;
		eth->rx_small.watchdog_needed = 0;
		eth->rx_big.watchdog_needed = 0;
		mx_ether_alloc_rx_pages(eth->is, &eth->rx_small,
					eth->arch.small_bytes, 0);
		if (eth->rx_small.fill_cnt < NUM_RX) {
			MX_WARN(("%s: mx_ether_open alloced only %d smalls\n",
				 dev->name, eth->rx_small.fill_cnt));
			goto abort_with_open;
		}

		mx_ether_alloc_rx_pages(eth->is, &eth->rx_big,
					dev->mtu + ETH_HLEN + VLAN_HLEN,
					0);
		if (eth->rx_big.fill_cnt < NUM_RX) {
			MX_WARN(("%s: mx_ether_open alloced only %d bigs\n",
				 dev->name, eth->rx_big.fill_cnt));
			goto abort_with_open;
		}
	} else {
		for (i = 0; i < NUM_RX; i++) {
			error = mx_get_buf(dev, eth, &eth->rx_small, i, 
					   eth->arch.small_bytes, GFP_KERNEL);
			if (error) {
				MX_WARN(("%s: Could not alloc small recv buffer %d, errno = %d\n",
					 dev->name, i, error));
				goto abort_with_open;
			}

			if (myri_ether_rx_frags) {
				error = mx_get_buf_big(eth, i, GFP_KERNEL);
			} else {
				error = mx_get_buf(dev, eth, &eth->rx_big, i, 
						   dev->mtu + ETH_HLEN + VLAN_HLEN,
						   GFP_KERNEL);
			}
			if (error) {
				MX_WARN(("%s: Could not alloc big recv buffer %d, errno = %d\n",
					 dev->name, i, error));
				goto abort_with_open;
			}
		}
		eth->rx_big.cnt = NUM_RX;
		eth->rx_small.cnt = NUM_RX;
		MX_PIO_WRITE(eth->rx_small.lanai_cnt, htonl(eth->rx_small.cnt));
		MX_PIO_WRITE(eth->rx_big.lanai_cnt, htonl(eth->rx_big.cnt));

	}
#ifdef HAVE_LRO
		lro_mgr = &eth->arch.lro_mgr;
		lro_mgr->dev = dev;
		lro_mgr->features = 0;
		lro_mgr->ip_summed = CHECKSUM_COMPLETE;
		lro_mgr->ip_summed_aggr = CHECKSUM_UNNECESSARY;
		lro_mgr->max_desc = MX_MAX_LRO_DESCRIPTORS;
		lro_mgr->lro_arr = eth->arch.lro_desc;
		if (myri_ether_rx_frags == 0) {
			lro_mgr->get_skb_header = mx_ether_get_skb_header;
			lro_mgr->max_aggr = mx_ether_lro_max_pkts;
		} else {
			lro_mgr->get_frag_header = mx_ether_get_frag_header;
			lro_mgr->max_aggr = mx_ether_lro_max_pkts;
			if (lro_mgr->max_aggr > MAX_SKB_FRAGS)
				lro_mgr->max_aggr = MAX_SKB_FRAGS;
		}
#ifdef HAVE_NEW_NAPI
		lro_mgr->features |= LRO_F_NAPI;	
#endif /* HAVE_NEW_NAPI */
#endif /* HAVE_LRO */

#ifdef HAVE_NEW_NAPI
	napi_enable(&eth->arch.napi); /* must happen before any irq */
#endif	
	/* flow control */
	mx_ether_set_pause(dev, eth->pause);

	/* tell the mcp about this */
	error = mx_mcp_command(eth->is, MCP_CMD_ETHERNET_UP,
				0, 0, 0, &dont_care);

	if (error) {
		MX_WARN(("%s: unable to start ethernet\n", dev->name));
		goto abort_with_open;
	}
	mx_ether_start_common(eth->is, dev->mtu + ETH_HLEN + VLAN_HLEN, 
			      eth->arch.small_bytes, mx_big_thresh);
	eth->running = MX_ETH_RUNNING;
	netif_wake_queue(dev);
	mx_ether_link_change_notify(eth->is);
	return 0;
	
abort_with_open:
#ifdef HAVE_NEW_NAPI
	napi_disable(&eth->arch.napi); 
#endif
	eth->running = MX_ETH_OPEN_FAILED;
	mx_ether_close(dev);
		
abort_with_nothing:
	eth->running = MX_ETH_STOPPED;
	return error;
}

static int
mx_ether_change_mtu (struct net_device *dev, int new_mtu)
{
	struct mx_ether *eth = mx_netdev_priv(dev);

	if ((new_mtu < 68) || (ETH_HLEN + new_mtu > MX_MAX_ETHER_MTU)) {
		MX_PRINT(("%s: new mtu (%d) is not valid\n",
			  dev->name, new_mtu));
		return -EINVAL;
	}
	MX_INFO(("%s: changing mtu from %d to %d\n",
		 dev->name, dev->mtu, new_mtu));
	if (eth->running && (new_mtu > dev->mtu)) {
		/* if we increase the mtu on an active device, we must
		   ensure that all buffers provided to the MCP are
		   of adequate length */
		mx_ether_close(dev);
		dev->mtu = new_mtu;
		mx_ether_open(dev);
	} else {
		dev->mtu = new_mtu;
	}
	return 0;
}

static struct net_device_stats *
mx_ether_get_stats(struct net_device *dev)
{
  struct mx_ether *eth = mx_netdev_priv(dev);
  return &eth->arch.stats;
}

static void
mx_ether_set_multicast_list(struct net_device *dev)
{
	struct mx_ether *eth = mx_netdev_priv(dev);
	struct myri_mc_list_type *ha;
	uint32_t dont_care, data[2] = {0, 0};
	int err;

	err = mx_ether_set_promisc_common(mx_netdev_priv(dev),
					  dev->flags & IFF_PROMISC);
	if (err != 0) {
		MX_WARN(("%s: failed to change Promiscuous state,"
			 " error status: %d\n", dev->name, err));
		goto abort;
	}

	/* Disable multicast filtering */

	err = mx_mcp_command(eth->is, MCP_CMD_ENABLE_ALLMULTI, 
			     0, 0, 0, &dont_care);
	if (err != 0) {
		MX_WARN(("%s: failed command ENABLE_ALLMULTI,"
			 " error status: %d\n", dev->name, err));
		goto abort;
	}

	if (dev->flags & IFF_ALLMULTI) {
		/* request to disable multicast filtering, so quit here */
		return;
	}

	/* Flush the filters */

	err = mx_mcp_command(eth->is, MCP_CMD_LEAVE_ALL_MULTICAST, 
			     0, 0, 0, &dont_care);
	if (err != 0) {
		MX_WARN(("%s: failed command LEAVE_ALL_MULTICAST,"
			 " error status: %d\n", dev->name, err));
		goto abort;
	}

	/* Walk the multicast list, and add each address */
	netdev_for_each_mc_addr(ha, dev) {
		memcpy(data, &ha->myri_mc_addr, 6);
		/* address is big endian */
		data[0] = htonl(data[0]);
		data[1] = htons(data[1]);
		err = mx_mcp_command(eth->is, MCP_CMD_JOIN_MULTICAST, 
				     0, data[0], data[1], &dont_care);
		if (err != 0) {
			MX_WARN(("%s: failed command JOIN_MULTICAST,"
			 " error status: %d\n", dev->name, err));
			/* printk(KERN_ERR "MAC " MYRI10GE_MAC_FMT "\n",
			   myri10ge_print_mac(mac, ha->myri_mc_addr)); */
			goto abort;
		}
	}
	/* Enable multicast filtering */
	err = mx_mcp_command(eth->is, MCP_CMD_DISABLE_ALLMULTI, 
			     0, 0, 0, &dont_care);
	if (err != 0) {
		MX_WARN(("%s: failed command DISABLE_ALLMULTI,"
			 " error status: %d\n", dev->name, err));
		goto abort;
	}

	return;

  abort:
	return;
}

void
mx_ether_timeout(struct net_device *dev)
{
	struct mx_ether *eth = mx_netdev_priv(dev);
	mx_instance_state_t *is;

	is = eth->is;
	if (!mx_is_dead(is)) {
		MX_WARN(("%s:mx_ether_timeout called, but lanai is running!\n",
			 dev->name));
		MX_WARN(("%s: tx req = 0x%x, tx done = 0x%x, NUM_TX = 0x%lx\n",
			 dev->name, eth->tx.req, eth->tx.done, NUM_TX));
	}
	if (dev->watchdog_timeo < HZ * 3600)
		dev->watchdog_timeo = 2 * dev->watchdog_timeo;
}

int
mx_ether_set_mac_address (struct net_device *dev, void *addr)
{
	struct sockaddr *sa = (struct sockaddr *) addr;
	struct mx_ether *eth = mx_netdev_priv(dev);


	/* change the dev structure */
	bcopy(sa->sa_data, dev->dev_addr, 6);
	mx_ether_set_mac_address_common(eth, dev->dev_addr);
	return 0;
}

#if LINUX_XX <= 24

static int
mx_ether_ethtool(struct net_device *dev, void *uva)
{
	struct ethtool_value val;
	struct mx_ether *eth = mx_netdev_priv(dev);
	mx_instance_state_t *is = eth->is;
	uint32_t cmd;
	int error = 0;

	if (copy_from_user(&cmd, uva, sizeof (cmd)))
		return -EFAULT;

	switch (cmd) {
	case ETHTOOL_GDRVINFO: 	{
		struct ethtool_drvinfo info;
		bzero(&info, sizeof (info));
		info.cmd = ETHTOOL_GDRVINFO;
		sprintf(info.driver, "%s ethernet", MYRI_DRIVER_STR);
		sprintf(info.version, "0x%x", MYRI_DRIVER_API_MAGIC);
		strcpy(info.bus_info, mx_pci_name(is->arch.pci_dev));
		if (copy_to_user(uva, &info, sizeof (info)))
			error = -EFAULT;
	}
	break;

#ifdef ETHTOOL_GLINK
	case ETHTOOL_GLINK:
		val.cmd = ETHTOOL_GLINK;
		val.data = is->link_state ? 1:0;
		if (copy_to_user(uva, &val, sizeof (val)))
			error = -EFAULT;
		break;
#endif		

#ifdef ETHTOOL_GRXCSUM
	case ETHTOOL_GRXCSUM: 
		val.cmd = ETHTOOL_GRXCSUM;
		val.data = (eth->csum_flag & MCP_ETHER_FLAGS_CKSUM) != 0;
		if (copy_to_user(uva, &val, sizeof (val)))
			error = -EFAULT;
		break;

	case ETHTOOL_SRXCSUM:
		if (copy_from_user(&val, uva, sizeof (val))) {
			error -= -EFAULT;
			goto abort;
		}
		if (val.data)
			eth->csum_flag |= MCP_ETHER_FLAGS_CKSUM;
		else
			eth->csum_flag &= ~MCP_ETHER_FLAGS_CKSUM;
		break;

	case ETHTOOL_GTXCSUM: 
		val.cmd = ETHTOOL_GTXCSUM;
		val.data = (dev->features & NETIF_F_IP_CSUM) != 0;
		if (copy_to_user(uva, &val, sizeof (val)))
			error = -EFAULT;
		break;

	case ETHTOOL_STXCSUM:
		if (copy_from_user(&val, uva, sizeof (val))) {
			error -= -EFAULT;
			goto abort;
		}
		if (val.data)
			dev->features |= NETIF_F_IP_CSUM;
		else
			dev->features &= ~NETIF_F_IP_CSUM;
		break;
#endif

#ifdef ETHTOOL_GSG
	case ETHTOOL_GSG: 
		val.cmd = ETHTOOL_GSG;
		val.data = (dev->features & NETIF_F_SG) != 0;
		if (copy_to_user(uva, &val, sizeof (val)))
			error = -EFAULT;
		break;

	case ETHTOOL_SSG:
		if (copy_from_user(&val, uva, sizeof (val))) {
			error -= -EFAULT;
			goto abort;
		}
		if (val.data)
			dev->features |= NETIF_F_SG;
		else
			dev->features &= ~NETIF_F_SG;
		break;
#endif
#ifdef ETHTOOL_GCOALESCE
		/* note that we don't distinguish between transmit and
		   receive interrupt coalescing.  Nor do we (yet) have
		   a concept of forcing an irq if more the N packets
		   have been received */
	case ETHTOOL_GCOALESCE: {
		struct ethtool_coalesce coal;

		bzero(&coal, sizeof (coal));
		coal.rx_coalesce_usecs = MCP_GETVAL(is, intr_coal_delay);
		coal.tx_coalesce_usecs = coal.rx_coalesce_usecs;
		if (copy_to_user(uva, &coal, sizeof (coal)))
			error = -EFAULT;
	} break;

	case ETHTOOL_SCOALESCE: {
		struct ethtool_coalesce coal;
		uint32_t old_val, new_val = 0;

		old_val = MCP_GETVAL(is, intr_coal_delay);
		if (copy_from_user(&coal, uva, sizeof (coal))) {
			error -= -EFAULT;
			goto abort;
		}

		if (coal.tx_coalesce_usecs != old_val)
			new_val = coal.tx_coalesce_usecs;
		else if (coal.rx_coalesce_usecs != old_val)
			new_val = coal.rx_coalesce_usecs;
		else {
			error = -EINVAL;
			goto abort;
		}
		
		if (new_val > 1000) {
			error = -EINVAL;
			goto abort;
		}
		MCP_SETVAL(is, intr_coal_delay, new_val);
	} break;
#endif /* ETHTOOL_GCOALESCE */

	default:
		error = -EOPNOTSUPP;
		break;
	}

abort:
	return error;

}

static int
mx_ether_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	int error = -EOPNOTSUPP;;

	switch (cmd) {

	case SIOCETHTOOL:
		error = mx_ether_ethtool(dev, (void *) ifr->ifr_data);
		break;

	/* put link-state checking here */

	default: 
		break;
	}

	return error;
}

#else
static int
mx_ether_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
	struct mx_ether *eth = mx_netdev_priv(netdev);
	cmd->autoneg = AUTONEG_DISABLE;
	if (MAL_IS_ZE_BOARD(eth->is->board_type)) {
		cmd->speed = SPEED_10000;
	} else {
		cmd->speed = 2000 * eth->is->num_ports;
	}
	cmd->duplex = DUPLEX_FULL;
	return 0;
}

static void
mx_ether_get_drvinfo(struct net_device *netdev,
		     struct ethtool_drvinfo *info)
{
	struct mx_ether *eth = mx_netdev_priv(netdev);
	mx_instance_state_t *is = eth->is;
	
	sprintf(info->driver, "%s ethernet", MYRI_DRIVER_STR);
	sprintf(info->version, "0x%x", MYRI_DRIVER_API_MAGIC);
	strlcpy(info->bus_info, mx_pci_name(is->arch.pci_dev), sizeof (info->bus_info));
}

int
mx_ether_get_coalesce(struct net_device *netdev,
		     struct ethtool_coalesce *coal)
{
	struct mx_ether *eth = mx_netdev_priv(netdev);
	mx_instance_state_t *is = eth->is;
	uint32_t delay;
	
	delay = MCP_GETVAL(is, intr_coal_delay);
	coal->rx_coalesce_usecs = delay;
	return 0;
}

int
mx_ether_set_coalesce(struct net_device *netdev,
		     struct ethtool_coalesce *coal)
{
	struct mx_ether *eth = mx_netdev_priv(netdev);
	mx_instance_state_t *is = eth->is;
	uint32_t delay;

	delay = coal->rx_coalesce_usecs;
	if (delay > 1000)
		return -EINVAL;

	MCP_SETVAL(is, intr_coal_delay, delay);
	return 0;
}

static void
mx_ether_get_pauseparam(struct net_device *netdev,
			struct ethtool_pauseparam *pause)
{
	struct mx_ether *eth = mx_netdev_priv(netdev);

	pause->autoneg = 0;
	pause->rx_pause = eth->pause;
	pause->tx_pause = eth->pause;
}

static int
mx_ether_set_pauseparam(struct net_device *netdev,
			struct ethtool_pauseparam *pause)
{
	struct mx_ether *eth = mx_netdev_priv(netdev);

	if (pause->tx_pause != eth->pause)
		return mx_ether_set_pause(netdev, pause->tx_pause);
	if (pause->rx_pause != eth->pause)
		return mx_ether_set_pause(netdev, pause->rx_pause);
	if (pause->autoneg != 0)
		return -EINVAL;
	return 0;
}

static u32
mx_ether_get_rx_csum(struct net_device *netdev)
{
	struct mx_ether *eth = mx_netdev_priv(netdev);

	if (eth->csum_flag & MCP_ETHER_FLAGS_CKSUM)
		return 1;
	else
		return 0;
}

static int
mx_ether_set_rx_csum(struct net_device *netdev, u32 csum_enabled)
{
	struct mx_ether *eth = mx_netdev_priv(netdev);

	if (csum_enabled)
		eth->csum_flag |= MCP_ETHER_FLAGS_CKSUM;
	else
		eth->csum_flag &= ~MCP_ETHER_FLAGS_CKSUM;
	return 0;
}

static u32
mx_ether_get_tx_csum(struct net_device *netdev)
{
	if ((netdev->features & NETIF_F_IP_CSUM) != 0)
		return 1;
	else
		return 0;
}

static int
mx_ether_set_tx_csum(struct net_device *netdev, u32 csum_enabled)
{
	if (csum_enabled)
		netdev->features |= NETIF_F_IP_CSUM;
	else
		netdev->features &= ~NETIF_F_IP_CSUM;
	return 0;
}



static const char mx_eth_gstrings[][ETH_GSTRING_LEN] = {
	"rx_packets", "tx_packets", "rx_bytes", "tx_bytes", "rx_errors",
	"tx_errors", "rx_dropped", "tx_dropped", "multicast", "collisions",
	"rx_length_errors", "rx_over_errors", "rx_crc_errors",
	"rx_frame_errors", "rx_fifo_errors", "rx_missed_errors",
	"tx_aborted_errors", "tx_carrier_errors", "tx_fifo_errors",
	"tx_heartbeat_errors", "tx_window_errors",
	/* device-specific stats */
	"rx_frags", "alloc_order", "link_up",
	"tx_req", "tx_done",
	"rx_small_cnt", "rx_big_cnt"
#ifdef HAVE_LRO
	, "LRO aggregated", "LRO flushed",
	"LRO avg aggr", "LRO no_desc"
#endif
	
};

#define MX_ETH_GSTRINGS_LEN  sizeof(mx_eth_gstrings) / ETH_GSTRING_LEN
#define MX_ETH_NET_STATS_LEN 21

static void
mx_ether_get_strings(struct net_device *netdev, u32 stringset, u8 *data)
{
	switch (stringset) {
	case ETH_SS_STATS:
		memcpy(data, *mx_eth_gstrings,
		       sizeof(mx_eth_gstrings));
		break;
	}
}

#ifndef HAVE_SSET_COUNT
static int
mx_ether_get_stats_count(struct net_device *netdev)
{
	return MX_ETH_GSTRINGS_LEN;
}
#else
static int
mx_ether_get_sset_count(struct net_device *netdev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return MX_ETH_GSTRINGS_LEN;
	default:
		return -EOPNOTSUPP;
	}
}
#endif /* HAVE_SSET_COUNT */

static void
mx_ether_get_ethtool_stats(struct net_device *netdev,
			   struct ethtool_stats *stats, u64 *data)
{
	struct mx_ether *eth = mx_netdev_priv(netdev);
	mx_instance_state_t *is = eth->is;

	int i;
	for (i = 0; i < MX_ETH_NET_STATS_LEN; i++)
		data[i] =  ((unsigned long *) &eth->arch.stats)[i];
	data[i++] = myri_ether_rx_frags;
	data[i++] = myri_ether_rx_alloc_order;
	data[i++] = is->link_state ? 1:0;
	data[i++] = (unsigned int)eth->tx.req;
	data[i++] = (unsigned int)eth->tx.done;
	data[i++] = (unsigned int)eth->rx_small.cnt;
	data[i++] = (unsigned int)eth->rx_big.cnt;
#ifdef HAVE_LRO
	data[i++] = eth->arch.lro_mgr.stats.aggregated;
	data[i++] = eth->arch.lro_mgr.stats.flushed;
	if (eth->arch.lro_mgr.stats.flushed)
		data[i++] = eth->arch.lro_mgr.stats.aggregated /
			eth->arch.lro_mgr.stats.flushed;
	else
		data[i++] = 0;
	data[i++] = eth->arch.lro_mgr.stats.no_desc;
#endif
}

static MX_ETHTOOL_OPS_TYPE mx_ethtool_ops = {
	.get_settings 			= mx_ether_get_settings,
	.get_drvinfo			= mx_ether_get_drvinfo,
	.get_coalesce			= mx_ether_get_coalesce,
	.set_coalesce			= mx_ether_set_coalesce,
	.get_pauseparam			= mx_ether_get_pauseparam,
	.set_pauseparam			= mx_ether_set_pauseparam,
	.get_rx_csum			= mx_ether_get_rx_csum,
	.set_rx_csum			= mx_ether_set_rx_csum,
	/* get_tx_csum, get_sg and get_tso are set by default since 2.6.24 */
	.get_tx_csum			= mx_ether_get_tx_csum,
	.set_tx_csum			= mx_ether_set_tx_csum,
	.get_sg				= ethtool_op_get_sg,
	.get_tso			= ethtool_op_get_tso,
	.set_sg				= ethtool_op_set_sg,
	.get_link			= ethtool_op_get_link,
	.get_strings			= mx_ether_get_strings,
#ifdef HAVE_SSET_COUNT
	.get_sset_count			= mx_ether_get_sset_count,
#else
	.get_stats_count		= mx_ether_get_stats_count,
#endif
	.get_ethtool_stats		= mx_ether_get_ethtool_stats,
};


#endif /* LINUX_XX */

static int
mx_ether_init(struct net_device *dev)
{
	return 0;
}


void
mx_ether_link_change_notify(mx_instance_state_t *is)
{
	if (is->ether == NULL || is->ether->running != MX_ETH_RUNNING)
		return;
	if (is->link_state)
		netif_carrier_on(is->ether->arch.dev);
	else
		netif_carrier_off(is->ether->arch.dev);
}

#ifdef HAVE_NET_DEVICE_OPS
static const struct net_device_ops mx_netdev_ops = {
	.ndo_init		= mx_ether_init,
	.ndo_open		= mx_ether_open,
	.ndo_stop		= mx_ether_close,
	.ndo_start_xmit		= mx_ether_xmit,
	.ndo_get_stats		= mx_ether_get_stats,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_change_mtu		= mx_ether_change_mtu,
	.ndo_set_multicast_list	= mx_ether_set_multicast_list,
	.ndo_set_mac_address	= mx_ether_set_mac_address,
	.ndo_tx_timeout		= mx_ether_timeout,
};
#endif


int 
mx_ether_attach(mx_instance_state_t *is)
{
	struct net_device *dev;
	struct mx_ether *eth;
	int err, i;

	if (myri_ether_rx_frags) {
		mx_ether_rx_alloc_size = (PAGE_SIZE << myri_ether_rx_alloc_order);
		if (mx_ether_rx_alloc_size > 65536) {
			MX_WARN(("%s: Alloc order %d too large\n",
				 is->is_name, myri_ether_rx_alloc_order));
			mx_ether_rx_alloc_size = PAGE_SIZE;
			myri_ether_rx_alloc_order = 0;
		}
	}
	if (myri_ether_ifname_legacy)
		dev = mx_netdev_alloc("myri%d", &err, is->id);
	else
		dev = mx_netdev_alloc("eth%d", &err, is->id);
	if (!dev) {
		MX_WARN(("%s: Alloc netdev failed\n", is->is_name));
		return err;
	}
	ether_setup(dev);
	eth = mx_netdev_priv(dev);
	eth->csum_flag = (myri_ether_csum ? MCP_ETHER_FLAGS_CKSUM : 0);
	eth->pause = myri_ether_pause;
	/* setup all the pointers.. */
	eth->is = is;
	eth->arch.dev = dev;
	MAL_STBAR();
	is->ether = eth;

	mx_ether_link_change_notify(is);
	dev->mtu = MX_MAX_ETHER_MTU - ETH_HLEN;
#ifdef HAVE_NET_DEVICE_OPS
	dev->netdev_ops = &mx_netdev_ops;
#else
	dev->open = mx_ether_open;
	dev->stop = mx_ether_close;
	dev->hard_start_xmit = mx_ether_xmit;
	dev->get_stats = mx_ether_get_stats;
	dev->change_mtu = mx_ether_change_mtu;
	dev->set_multicast_list = mx_ether_set_multicast_list;
	dev->set_mac_address = mx_ether_set_mac_address;
	dev->init = mx_ether_init;
	dev->tx_timeout = mx_ether_timeout;
#endif
	dev->base_addr = pci_resource_start(is->arch.pci_dev, 0);
	dev->irq = is->arch.irq;
	dev->watchdog_timeo = HZ * 2;
#if LINUX_XX <= 24
	dev->do_ioctl = mx_ether_ioctl;
#else
	SET_ETHTOOL_OPS(dev, &mx_ethtool_ops);
#endif
	for (i = 0; i < 6; i++) {
		eth->current_mac[i] = dev->dev_addr[i]= is->mac_addr[i];
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14)
		dev->perm_addr[i]= is->mac_addr[i];
#endif
	}

	dev->features = NETIF_F_SG| (myri_ether_csum ? NETIF_F_IP_CSUM : 0) | NETIF_F_HIGHDMA;

	mx_ether_rx_alloc_size = PAGE_SIZE << myri_ether_rx_alloc_order;
	SET_NETDEV_DEV(dev, &is->arch.pci_dev->dev);
	eth->arch.fake_page = alloc_pages(GFP_KERNEL, myri_ether_rx_alloc_order);
	
	if ((err = register_netdev(dev)) || eth->arch.fake_page == NULL) {
		if (err != 0)
			MX_WARN(("%s:register_netdev failed!! (err = %d)\n", dev->name, err));
		else {
			MX_WARN(("%s:fake page alloc failed!!\n", dev->name));
			err = -ENOMEM;
		}
		if (eth->arch.fake_page != NULL)
			put_page(eth->arch.fake_page);
		mx_netdev_free(dev);
		is->ether = NULL;
		return err;
	}
        if (strcmp(dev->name, is->is_name) != 0) {
          MX_INFO(("%s: device now known as %s\n", is->is_name, dev->name));
        }
	is->is_name = dev->name;
	if (myri_ether_rx_frags) {
		MX_INFO(("%s: Will use skbuf frags (%d bytes, order=%d)\n",
			 is->is_name, mx_ether_rx_alloc_size,
			 myri_ether_rx_alloc_order));
        }
#ifdef HAVE_NEW_NAPI
	netif_napi_add(dev, &eth->arch.napi, mx_ether_poll, mx_ether_napi_weight);
#endif	
	return (0);
}

void
mx_ether_detach(mx_instance_state_t *is)
{
	struct net_device *dev;
	struct mx_ether *eth;

	
	eth = is->ether;
	if (!eth)
		return;
	dev = eth->arch.dev;
        is->is_name = is->is_name_default;
	unregister_netdev(dev);
	put_page(eth->arch.fake_page);
	is->ether = 0;
	MAL_STBAR();
	mx_netdev_free(dev);
}


int
mx_ether_parity_detach(mx_instance_state_t *is)
{
	struct mx_ether *eth = is->ether;

	if (!eth)
		return 0;

	if (eth->arch.dev->flags & IFF_UP) {
		netif_carrier_off(eth->arch.dev);
		MX_WARN(("Detaching %s\n", is->is_name));
		mx_ether_close(eth->arch.dev);
		return 1;
	}
	return 0;
}

void
mx_ether_parity_reattach(mx_instance_state_t *is)
{
	struct mx_ether *eth = is->ether;

	MX_WARN(("re-attaching %s\n", is->is_name));
	mx_ether_open(eth->arch.dev);
}

void
mx_ether_watchdog(mx_instance_state_t *is)
{
	struct mx_ether *eth = is->ether;
	mx_ether_rx_buf_t *rx;

	if (!myri_ether_rx_frags)
		return;

	if (!(eth->arch.dev->flags & IFF_UP))
		return;

	rx = &eth->rx_small;
	if (rx->watchdog_needed) {
		mx_ether_alloc_rx_pages(is, rx, eth->arch.small_bytes,
					1);
		if (rx->fill_cnt - rx->cnt)
			rx->watchdog_needed = 0;
	}

	rx = &eth->rx_big;
	if (rx->watchdog_needed) {
		mx_ether_alloc_rx_pages(is, rx, 
					eth->arch.dev->mtu + ETH_HLEN + VLAN_HLEN,
					1);
		if (rx->fill_cnt - rx->cnt)
			rx->watchdog_needed = 0;
	}
}

int
myri_ether_soft_rx(mx_instance_state_t *is, myri_soft_rx_t *r)
{
	struct sk_buff *skb = NULL;
	char *uaddr;
	int retval = -EBADF;
	int hlen, copylen, buf, copy;


	if (r->pkt_len > 9018)
		return EINVAL;

	if (r->flags & MYRI_SOFT_RX_FAKE)
		skb = alloc_skb(r->hdr_len, GFP_KERNEL);
	else
		skb = alloc_skb(r->pkt_len, GFP_KERNEL);
	if (skb == NULL) {
		retval = ENOMEM;
		goto bad;
	}

	/* avoid checksum on copyin for simplicity */
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	copylen = r->pkt_len;	
	/* copy header separately, if required */
	buf = 0;
	if ((r->flags &  MYRI_SOFT_RX_CSUM_PART) ||
	    (r->flags & MYRI_SOFT_RX_FAKE)) {
		hlen = r->hdr_len;
		if (hlen > copylen) {
			retval = EINVAL;
			goto bad;
		}
		while (hlen != 0) {
			copy = min(hlen, (int)r->copy_desc[buf].len);
			uaddr = (char *)(unsigned long)r->copy_desc[buf].ptr;
			retval = skb_add_data(skb, uaddr, copy);
			if (retval)
				goto bad;
			hlen -= copy;
			copylen -= copy;
			r->copy_desc[buf].len -= copy;
			if (r->copy_desc[buf].len == 0)
				buf++;
			if (buf > 2) {
				printk("error, too many bufs!\n");
				retval = EINVAL;
				goto bad;
			}
		}
	}

	if ((r->flags & MYRI_SOFT_RX_FAKE)) {
		buf = 0;
		while (copylen != 0) {
			copy = min(copylen, mx_ether_rx_alloc_size);
			skb_shinfo(skb)->nr_frags++;
			get_page(is->ether->arch.fake_page);
			skb_shinfo(skb)->frags[buf].page = is->ether->arch.fake_page;
			skb_shinfo(skb)->frags[buf].page_offset = 0;
			skb_shinfo(skb)->frags[buf].size = copy;
			skb->data_len += copy;
			skb->len += copy;
			copylen -= copy;
			buf++;
		}
	} else {
		/* copy the rest of the packet */
		while (copylen != 0) {
			copy = min(copylen, (int)r->copy_desc[buf].len);
			uaddr = (char *)(unsigned long)r->copy_desc[buf].ptr;
			retval = skb_add_data(skb, uaddr, copy);
			if (retval)
				goto bad;
			copylen -= copy;
			r->copy_desc[buf].len -= copy;
			if (r->copy_desc[buf].len == 0)
				buf++;
			if (buf > 2) {
				printk("error, too many bufs!\n");
				retval = EINVAL;
				goto bad;
			}
		}
	}

	switch (r->flags & MYRI_SOFT_CSUM_MASK) {

	case  MYRI_SOFT_RX_CSUM_PART:
		skb->ip_summed = CHECKSUM_COMPLETE;
		skb->csum = r->csum;
		break;

	case  MYRI_SOFT_RX_CSUM_GOOD:
	case  MYRI_SOFT_RX_FAKE:
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		break;
	default:
		skb->ip_summed = CHECKSUM_NONE;
		break;
	}


	skb->truesize = r->pkt_len + sizeof (*skb);
	skb->protocol = eth_type_trans(skb, is->ether->arch.dev);
	skb->dev = is->ether->arch.dev;
	netif_rx_ni(skb);
	return (0);

bad:
	if (skb != NULL)
		dev_kfree_skb(skb);
	return (retval);
}

/*
  This file uses MX driver indentation.

  Local Variables:
  c-file-style:"linux"
  tab-width:8
  End:
*/
