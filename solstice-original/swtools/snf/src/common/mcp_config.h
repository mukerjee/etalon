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

#ifndef _mcp_config_h_
#define _mcp_config_h_

#include "mal_auto_config.h"


#define MCP_NODES_CNT		512
#define MCP_ENDPOINTS_CNT	32
#define MCP_ENDPOINTS_MAX	64
#define MCP_SEND_HANDLES_CNT	63
#define MCP_PUSH_HANDLES_CNT	256
#define MCP_RDMA_WINDOWS_CNT	256

#define MCP_RECVQ_VPAGE_CNT	512
#define MCP_EVENTQ_VPAGE_CNT	8
#define MCP_SENDQ_VPAGE_CNT	1024

#define MCP_ETHER_MAX_SEND_FRAG	12
#define MCP_ETHER_MIN_RECV_FRAG	8
#define MCP_ETHER_PAD		6

#define MCP_KREQQ_CNT		128   /* 4KB */
#define MCP_UREQQ_CNT		(MCP_SEND_HANDLES_CNT + 1)
#define MCP_UDATAQ_SIZE		(MCP_UMMAP_SIZE - (MCP_UREQQ_CNT * 64) - 8)

#define MCP_ROUTE_CNT_BLOCK	8
#define MCP_ROUTE_MAX_LENGTH	7
#define MCP_RAW_TX_CNT		64
#define MCP_RAW_RX_CNT		128
#define MCP_RAW_MTU		1024

#define MCP_GLOBAL_OFFSET	4096

#define MCP_PRINT_BUFFER_SIZE	(256 + (MAL_DEBUG * 768))

#define MCP_INTR_COAL_DELAY	100

#define	MCP_UMMAP_SIZE		(16 * 1024)  /* 16 K pages for IA-64 */


#if MX_2K_PAGES
#define MX_VPAGE_SHIFT		11 /* 2048 */
#else
#define MX_VPAGE_SHIFT		12 /* 4096 */
#endif
#define MX_VPAGE_SIZE		(1UL << MX_VPAGE_SHIFT)
#define MX_VPAGE_OFFSET(x)	((x) & (MX_VPAGE_SIZE - 1))
#define MX_VPAGE_TRUNC(x)	((x) & ~(MX_VPAGE_SIZE - 1))
#define MX_VPAGE_ALIGN(x)	(MX_VPAGE_TRUNC((x) + MX_VPAGE_SIZE - 1))
#define MX_VPAGE_CREDITS(x)	(((x) + (MX_VPAGE_SIZE - 1)) / MX_VPAGE_SIZE)

#define MX_ETHER_TX_SIZE	(4*MX_VPAGE_SIZE)
#define MX_ETHER_RX_SIZE	(2*MX_VPAGE_SIZE)

#define MX_MMU_HASH	4096
#define MX_MMU_CNT	2048
#define MX_MMU_BLOCK	8


#if MX_RDMA_2K
#define MCP_TX_BOUNDARY 2048
#else
#define MCP_TX_BOUNDARY MX_VPAGE_SIZE
#endif

#endif  /* _mcp_config_h_ */
