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

#ifndef _mcp_global_h_
#define _mcp_global_h_

#include "mcp_config.h"
#include "mcp_types.h"

#define MCP_STATUS_LOAD		0xBEEF1111
#define MCP_STATUS_INIT		0xBEEF2222
#define MCP_STATUS_RUN		0xBEFF3333
#define MCP_STATUS_PARITY	0xBEEF4444
#define MCP_STATUS_ERROR	0xBEEF5555

#define MCP_PARITY_NONE		0xCACA0000
#define MCP_PARITY_MULTIPLE	0xCACA1111
#define MCP_PARITY_PANIC	0xCACA2222
#define MCP_PARITY_RECOVER_NONE	0xCACA3333
#define MCP_PARITY_RECOVER_ALL	0xCACA4444

#define MX_DMA_INVALID_ENTRY	0xffffffff
#define MX_DMA_BOGUS_ENTRY	0x87654321

/* Commands */
typedef enum {
  MCP_CMD_NONE = 0,
  MCP_CMD_RESET_COUNTERS,
  MCP_CMD_OPEN_ENDPOINT,
  MCP_CMD_SET_ENDPOINT_SESSION,
  MCP_CMD_GET_ENDPT_MAP_OFFSET,
  MCP_CMD_GET_USER_MMAP_OFFSET,
  MCP_CMD_GET_RDMA_WINDOWS_OFFSET,
  MCP_CMD_GET_HWCLOCK_OFFSET,
  MCP_CMD_ENABLE_ENDPOINT,
  MCP_CMD_CLOSE_ENDPOINT,
  MCP_CMD_ADD_PEER,
  MCP_CMD_REMOVE_PEER,
  MCP_CMD_GET_ROUTES_OFFSET,
  MCP_CMD_UPDATE_ROUTES,
  MCP_CMD_ETHERNET_UP,
  MCP_CMD_ETHERNET_DOWN,
  MCP_CMD_ENABLE_FLOW_CONTROL,
  MCP_CMD_DISABLE_FLOW_CONTROL,
  MCP_CMD_ENABLE_PROMISC,
  MCP_CMD_DISABLE_PROMISC,
  MCP_CMD_ENABLE_ALLMULTI,
  MCP_CMD_DISABLE_ALLMULTI,
  MCP_CMD_JOIN_MULTICAST,
  MCP_CMD_LEAVE_MULTICAST,
  MCP_CMD_LEAVE_ALL_MULTICAST,
  MCP_CMD_START_LOGGING,
  MCP_CMD_START_DMABENCH_READ,
  MCP_CMD_START_DMABENCH_WRITE,
  MCP_CMD_CLEAR_RAW_STATE,
  MCP_CMD_SET_DISPERSION,
  MCP_CMD_GET_DISPERSION,
  MCP_CMD_SNF_RING,
  MCP_CMD_SNF_RECV,
} mcp_cmd_type_t;

typedef enum {
  MCP_CMD_OK = 0,
  MCP_CMD_UNKNOWN,
  MCP_CMD_ERROR_RANGE,
  MCP_CMD_ERROR_BUSY,
  MCP_CMD_ERROR_CLOSED,
} mcp_cmd_status_t;

/* link_state word related definitions:
   reserve room for up to 16 port per NIC, (in practice at most 2)
    - 16 first bits of word represent the up status of each   port,
    - 16 upper bits are to signal a network mismatch
*/
#define MCP_LINK_STATE_UP 1
#define MCP_LINK_STATE_MISMATCH 0x10000


typedef struct
{
  /* Misc */
  uint32_t mcp_status;             /* Current status of the MCP. */
  uint32_t parity_status;
  uint32_t parity_address;
  uint32_t reboot_status;
  uint32_t params_ready;
  
  /* Parameters */
  uint32_t nodes_cnt;              /* Number of NICs on the fabric. */
  uint32_t endpoints_cnt;          /* Number of user endpoints. */
  uint32_t send_handles_cnt;       /* Number of Send handles per endpoints. */
  uint32_t push_handles_cnt;       /* Number of Push handles. */
  uint32_t rdma_windows_cnt;
  uint32_t intr_coal_delay;        /* Minimum delay between interrupts (us). */
  uint32_t mac_high32;
  uint32_t mac_low16;
  uint32_t mac_high16;
  uint32_t mac_low32;
  uint32_t ethernet_mtu;
  uint32_t ethernet_smallbuf;
  uint32_t ethernet_bigbuf;
  uint32_t random_seed;
  uint32_t endpt_recovery;
  uint32_t recvq_vpage_cnt;
  uint32_t eventq_vpage_cnt;
  
  char hostname[128];    /* Must be 8-byte aligned */
  
  mcp_dma_addr_t endpt_desc_dma;

  /* Interface MCP/Driver */
  mcp_dma_addr_t intr_dma;
  mcp_dma_addr_t raw_rx_data[MCP_RAW_RX_CNT * MCP_RAW_MTU / MX_VPAGE_SIZE];
  mcp_dma_addr_t raw_desc;
  mcp_dma_addr_t host_query_vpage;
  mcp_dma_addr_t tx_done_vpage;
  mcp_dma_addr_t sysbox_vpage;
  mcp_dma_addr_t counters_vpage;
  mcp_dma_addr_t bogus_vpage;
  uint32_t kreq_host_cnt;
  uint32_t raw_recv_enabled;
  uint32_t raw_rx_cnt;
  uint32_t fma_nic_reply_offset;
  uint32_t fma_nic_reply_update;
  uint32_t snf_cnt_recv;
  uint32_t snf_cnt_drop_flow;
  uint32_t snf_rx_doorbell_off;
  
  /* ethernet */
  volatile uint8_t ether_tx_ring[MX_ETHER_TX_SIZE];
  volatile uint8_t ether_rx_small_ring[MX_ETHER_RX_SIZE];
  volatile uint8_t ether_rx_big_ring[MX_ETHER_RX_SIZE];
  volatile uint32_t ether_tx_cnt;
  volatile uint32_t ether_rx_small_cnt;
  volatile uint32_t ether_rx_big_cnt;
  mcp_dma_addr_t ether_rx_desc_dma[MX_ETHER_RX_DESC_VPAGES];
  
  uint32_t kreqq_offset;
  uint32_t counters_offset;
  uint32_t mmu_hash_offset;
  uint32_t mmu_table_offset;
  uint32_t logging_offset;
  uint32_t print_buffer_addr;      /* Address of printf buffer */
  uint32_t print_buffer_pos;       /* Current index in printf buffer */
  volatile uint32_t print_buffer_limit;     /* Limit of printf buffer */
  uint32_t clock_freq;
  uint32_t local_peer_index;
  uint32_t dma_dbg_lowest_addr;
  uint32_t dma_dbg_pfn_max;
  uint32_t z_loopback;
  uint32_t pcie_down_on_error;
  uint32_t lxgdb;
  uint32_t pcie_rx_detect;
  uint32_t pcie_lane_select;
  uint32_t host_page_size;
  uint32_t ether_pause;
} mcp_public_global_t;


#endif  /* _mcp_global_h_ */
