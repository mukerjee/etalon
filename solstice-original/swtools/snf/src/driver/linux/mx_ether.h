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

#ifndef _mx_ether_h_
#define _mx_ether_h_

#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/string.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/etherdevice.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/pci.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/ip.h>
#include <linux/inet.h>
#include <linux/ethtool.h>
#include <net/checksum.h>
#ifdef HAVE_LRO
#include <linux/inet_lro.h>
#endif

#if (LINUX_VERSION_CODE < 0x020412)
#define DECLARE_PCI_UNMAP_ADDR(ADDR_NAME)
#define DECLARE_PCI_UNMAP_LEN(LEN_NAME)
#define pci_unmap_addr(PTR, ADDR_NAME)          0
#define pci_unmap_addr_set(PTR, ADDR_NAME, VAL) do{} while(0)
#define pci_unmap_len(PTR, LEN_NAME)            0
#define pci_unmap_len_set(PTR, LEN_NAME, VAL)   do{} while(0)
#endif


/* round skb allocations up to a 16 byte boundary */
#define SKB_ROUNDUP(x) (((x) + 15) & ~15)

/* XXX this should be elsewhere */
#if MAL_CPU_powerpc64  
#define INVALID_DMA_ADDR ((dma_addr_t)-1)
#else
#define INVALID_DMA_ADDR ((dma_addr_t)0)
#endif

#define MX_MAX_LRO_DESCRIPTORS 8

struct mx_ether_buffer_info {
	union {
		struct sk_buff *skb;
		struct mx_ether_page_info {
			struct page *page;
			int page_offset;
		} pg_info;
	} u;
	DECLARE_PCI_UNMAP_ADDR(bus)
	DECLARE_PCI_UNMAP_LEN(len)
};

struct mx_ether_arch {
	struct net_device *dev;				/* ethernet driver.	*/
	struct net_device_stats stats;			/* network stats 	*/
	int small_bytes;
#ifdef HAVE_LRO
	struct net_lro_mgr lro_mgr;
	struct net_lro_desc lro_desc[MX_MAX_LRO_DESCRIPTORS];
#endif
#ifdef HAVE_NEW_NAPI
	struct napi_struct napi;
#endif
	struct page *fake_page;
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#define mx_netdev_priv(d) *(struct mx_ether **)(netdev_priv(d))
#else
#define mx_netdev_priv(d) ((d)->priv)
#endif

#ifdef HAVE_NEW_NAPI
#define mx_rx_skb      netif_receive_skb
#else
#define mx_rx_skb      netif_rx
#endif

#endif /* _mx_ether_h_ */

/*
  This file uses MX driver indentation.

  Local Variables:
  c-file-style:"linux"
  tab-width:8
  End:
*/
