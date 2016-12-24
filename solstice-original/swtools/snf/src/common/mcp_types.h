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

#ifndef _mcp_types_h_
#define _mcp_types_h_

#define MCP_SYSBOX_OFF_CMD	0

#define MX_DMA_ADDR_SHIFT 3

/* 8 Bytes */
typedef struct
{
  uint32_t high;
  uint32_t low;
} mcp_dma_addr_t;

typedef struct
{
  uint32_t result;
  uint32_t status;
} mcp_cmd_done_t;

/* 80 Bytes */
typedef struct
{
  /* 16 Bytes */
  uint32_t next;
  uint32_t pad;
  uint32_t key_high;
  uint32_t key_low;
  
  /* 64 Bytes */
  mcp_dma_addr_t addr[8];
} mcp_mmu_t;

/* 32 Bytes */
typedef struct
{
  uint32_t length;
  uint32_t virt_high;
  uint32_t virt_low;
  uint32_t log_level;
  uint32_t seqnum;
  uint32_t refcnt;
  mcp_dma_addr_t table;
} mcp_rdma_win_t;


#define MX_ETHER_RX_DESC_VPAGES						\
  (2 * MX_ETHER_RX_SIZE / sizeof(mcp_dma_addr_t)			\
   * sizeof(mcp_ether_rx_desc_t) / MX_VPAGE_SIZE)

#define MCP_ETHER_RX_SMALL	1
#define MCP_ETHER_RX_BIG	2

/* 8 Bytes */
typedef struct
{
  uint16_t cksum;
  uint16_t len;
  uint8_t buf_cnt;
  uint8_t flags;
  uint8_t dmaop_cnt;
  uint8_t type;
#if MYRI_ENABLE_PTP
  uint32_t nticks_high;
  uint32_t nticks_low;
#endif
} mcp_ether_rx_desc_t;


#define MCP_RAW_TYPE_NONE	0
#define MCP_RAW_TYPE_TX		1
#define MCP_RAW_TYPE_RX		2

/* 8 Bytes */
typedef struct
{
  uint32_t length;
  uint8_t port;
  uint8_t pad;
  uint8_t tx_id;
  uint8_t type;
#if MYRI_ENABLE_PTP
  uint32_t nticks_high;
  uint32_t nticks_low;
#endif
} mcp_raw_desc_t;


#define MCP_INTR_ETHER_TX	(1 << 0)
#define MCP_INTR_ETHER_RX	(1 << 1)
#define MCP_INTR_ETHER_DOWN	(1 << 2)
#define MCP_INTR_LINK_STATE	(1 << 3)
#define MCP_INTR_DMABENCH	(1 << 4)
#define MCP_INTR_LOGGING	(1 << 5)
#define MCP_INTR_ENDPT		(1 << 6)
#define MCP_INTR_RAW		(1 << 7)
#define MCP_INTR_QUERY		(1 << 8)
#define MCP_INTR_SNF_WAKE	(1 << 9)

/* 32 Bytes */
typedef struct
{
  uint32_t ether_tx_cnt;
  uint32_t link_state;
  uint16_t dmabench_cpuc;
  uint16_t dmabench_cnt;
  uint16_t query_peer_index;
  uint16_t logging_size;
  uint32_t fcoe_data0;
  uint32_t fcoe_data1;
  uint32_t flags;
  uint16_t seqnum;
  uint16_t valid;
} mcp_intr_t;

/* 16 Bytes */
typedef struct
{
  uint32_t data0;
  uint32_t data1;
  uint32_t seqnum;
  uint16_t index;
  uint8_t flag;
  uint8_t type;
} mcp_slot_t;


#define MCP_ENDPT_WAKE_RX	1
#define MCP_ENDPT_WAKE_TX	2
#define MCP_ENDPT_CLOSED	3
#define MCP_ENDPT_ERROR		4
#define MCP_ENDPT_HELPER       	5

/* 4 Bytes */
typedef struct
{
  uint8_t index;
  uint8_t type;
  uint8_t pad;
  uint8_t valid;
} mcp_endpt_desc_t;
  
/* 16 Bytes */
typedef struct {
  uint32_t  nticks_high;
  uint32_t  nticks_low;
  uint32_t  pad;
  uint32_t  nticks_seqnum;
} mcp_hwclock_update_t;





#define SNF_TX_DESC_MAX         32
#define SNF_TX_DESC_PKT_MAX	64
#define SNF_TX_DESC_PKT_ALIGN	8

#define SNF_RX_DESC_PKT_MAX	((128 - 12) / 2)
#define SNF_RX_DESC_SEQNUM_MAX	255

#define SNF_TX_INTR 1
#define SNF_RX_INTR 2

/* Format in FW and lib as eventq elem */
typedef struct {
 uint16_t pkt_lens[SNF_RX_DESC_PKT_MAX];
 uint32_t timestamp_high;
 uint32_t timestamp_low;
 uint16_t total_len;
 uint8_t pkt_cnt;
 uint8_t seqnum;
} mcp_snf_rx_desc_t;


#endif /* _mcp_types_h_ */
