/*************************************************************************
 * The contents of this file are subject to the MYRICOM ABSTRACTION      *
 * LAYER (MAL) SOFTWARE AND DOCUMENTATION LICENSE (the "License").       *
 * User may not use this file except in compliance with the License.     *
 * The full text of the License can found in LICENSE.TXT                 *
 *                                                                       *
 * Software distributed under the License is distributed on an "AS IS"   *
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See  *
 * the License for the specific language governing rights and            *
 * limitations under the License.                                        *
 *                                                                       *
 * Copyright 2003 - 2010 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#ifndef _mal_io_h_
#define _mal_io_h_

#if defined HAVE_SYS_ERRNO_H && !defined MAL_KERNEL && !defined MX_MCP
#include <sys/errno.h>
#endif

#if defined HAVE_SYS_TYPES_H && !defined MAL_KERNEL && !defined MX_MCP
#include <sys/types.h>
#endif

#ifdef WIN32
#ifdef MAL_KERNEL
#include <devioctl.h>
#else
#include <windows.h>
#include <winioctl.h>
#endif
#endif


#include "mx_arch_io.h"
#include "myri_clksync.h"

#ifndef MYRI_IO
#define MYRI_IO(command) (('M' << 8) + (command))
#endif

/* driver/lib/fw interface
   the LSB increase for minor backwards compatible change,
   the MSB increase for incompatible change */
#define MYRI_DRIVER_API_MAGIC 0x100

/* MYRI ioctls are shared by multiple driver variants any incompatible 
 * changes in the MYRI ioctls requires that MSB in other driver API magic
 * be updated.
 *
 * Also, don't renumber ioctls.  This means don't add one in the middle, and if
 * you remove one, mark it as unused.  
 * Don't forget to update the MYRI_NUM_IOCTLS define 
 */

/* MYRI gets 0-99 */
#define MYRI_GET_VERSION			MYRI_IO(1)
#define MYRI_GET_BOARD_MAX			MYRI_IO(2)
#define MYRI_GET_BOARD_COUNT			MYRI_IO(5)

#define MYRI_GET_BOARD_TYPE     		MYRI_IO(10)
#define MYRI_GET_BOARD_NUMA_NODE		MYRI_IO(11)
#define MYRI_GET_PORT_COUNT			MYRI_IO(12)
#define MYRI_GET_NIC_ID				MYRI_IO(13)

#define MYRI_MMAP				MYRI_IO(15)

#define MYRI_GET_INTR_COAL			MYRI_IO(20)
#define MYRI_SET_INTR_COAL			MYRI_IO(21)
#define MYRI_GET_COUNTERS_CNT			MYRI_IO(30)
#define MYRI_GET_COUNTERS_STR			MYRI_IO(31)
#define MYRI_GET_COUNTERS_VAL			MYRI_IO(32)
#define MYRI_GET_COUNTERS_IRQ			MYRI_IO(33)
#define MYRI_GET_COUNTERS_KB			MYRI_IO(34)
#define MYRI_CLEAR_COUNTERS			MYRI_IO(35)
#define MYRI_GET_LOGGING			MYRI_IO(40)

#define MYRI_GET_SRAM_SIZE			MYRI_IO(45)

#define MYRI_PCI_CFG_READ			MYRI_IO(48)
#define MYRI_PCI_CFG_WRITE			MYRI_IO(49)

#define MYRI_GET_ENDPT_MAX			MYRI_IO(50)
#define MYRI_GET_ENDPT_OPENER			MYRI_IO(51)
#define MYRI_SOFT_RX				MYRI_IO(52)

#define MYRI_RAW_GET_PARAMS	 	 	MYRI_IO(90)
#define MYRI_RAW_SEND	 	 	 	MYRI_IO(91)
#define MYRI_RAW_GET_NEXT_EVENT			MYRI_IO(92)

#if MAL_OS_UDRV
#define MYRI_UDRV_DOORBELL			MYRI_IO(60)
#endif

#if MYRI_ENABLE_PTP
#define MYRI_PTP_RX_AVAIL			MYRI_IO(70)
#define MYRI_PTP_GET_RX_MSG			MYRI_IO(71)
#define MYRI_PTP_STORE_RX_MSG			MYRI_IO(72)
#define MYRI_PTP_START_STOP			MYRI_IO(73)
#define MYRI_PTP_PUT_TX_TIMESTAMP		MYRI_IO(74)
#define MYRI_PTP_GET_TX_TIMESTAMP		MYRI_IO(75)
#endif



#define MX_SET_ENDPOINT				MYRI_IO(910)
#define MX_WAIT                                 MYRI_IO(911)
#define MX_GET_COPYBLOCKS                       MYRI_IO(913)
#define MX_REGISTER                          	MYRI_IO(917)
#define MX_DEREGISTER                           MYRI_IO(918)
#define MX_GET_MAX_SEND_HANDLES			MYRI_IO(919)
#define MX_PIN_SEND                             MYRI_IO(920)
#define MX_NIC_ID_TO_BOARD_NUM                  MYRI_IO(921)
#define MX_NIC_ID_TO_PEER_INDEX                 MYRI_IO(925)
#define MX_GET_MAPPER_MSGBUF_SIZE               MYRI_IO(926)
#define MX_GET_MAPPER_MAPBUF_SIZE               MYRI_IO(927)
#define MX_GET_MAPPER_MSGBUF                    MYRI_IO(928)
#define MX_GET_MAPPER_MAPBUF                    MYRI_IO(929)
#define MX_GET_PEER_FORMAT                      MYRI_IO(930)
#define MX_GET_ROUTE_SIZE                       MYRI_IO(931)
#define MX_GET_PEER_TABLE                       MYRI_IO(932)
#define MX_GET_ROUTE_TABLE                      MYRI_IO(933)
#define MX_GET_MAX_PEERS                        MYRI_IO(934)
#define MX_PAUSE_MAPPER                         MYRI_IO(935)
#define MX_RESUME_MAPPER                        MYRI_IO(936)
#define MX_GET_SERIAL_NUMBER                    MYRI_IO(937)
#define MX_NIC_ID_TO_HOSTNAME			MYRI_IO(941)
#define MX_HOSTNAME_TO_NIC_ID			MYRI_IO(942)
#define MX_CLEAR_PEER_NAMES			MYRI_IO(943)
#define MX_SET_HOSTNAME				MYRI_IO(944)
#define MX_GET_CACHELINE_SIZE			MYRI_IO(945)
#define MX_GET_MAX_EVENT_SIZE      		MYRI_IO(946)
#define MX_GET_SMALL_MESSAGE_THRESHOLD		MYRI_IO(947)
#define MX_GET_LINK_STATE		 	MYRI_IO(948)
#define MX_GET_MEDIUM_MESSAGE_THRESHOLD		MYRI_IO(949)
#define MX_SET_ROUTE		 		MYRI_IO(950)
#define MX_SET_ROUTE_BEGIN	 		MYRI_IO(951)
#define MX_SET_ROUTE_END	 		MYRI_IO(952)
#define MX_PIN_RECV                             MYRI_IO(953)
#define MX_GET_MAX_RDMA_WINDOWS			MYRI_IO(954)
#define MX_RUN_DMABENCH			 	MYRI_IO(955)
#define MX_CLEAR_WAIT			 	MYRI_IO(956)
#define MX_WAKE			 	 	MYRI_IO(957)
#define MX_CLEAR_ROUTES				MYRI_IO(959)
#define MX_GET_BOARD_STATUS 	 	 	MYRI_IO(960)
#define MX_GET_CPU_FREQ 	 	 	MYRI_IO(961)
#define MX_GET_PCI_FREQ 	 	 	MYRI_IO(962)
#define MX_SET_RAW	 	 	 	MYRI_IO(963)
#define MX_SET_MAPPER_STATE			MYRI_IO(966)
#define MX_GET_MAPPER_STATE			MYRI_IO(967)
#define MX_RECOVER_ENDPOINT			MYRI_IO(970)
#define MX_REMOVE_PEER				MYRI_IO(971)
#define MX_PEER_INDEX_TO_NIC_ID			MYRI_IO(972)
#define MX_DIRECT_GET     			MYRI_IO(973)
#define MX_WAKE_ENDPOINT     			MYRI_IO(974)
#define MX_GET_PRODUCT_CODE    			MYRI_IO(975)
#define MX_GET_PART_NUMBER    			MYRI_IO(976)
#define MX_APP_WAIT				MYRI_IO(977)
#define MX_APP_WAKE				MYRI_IO(978)
#define MX_DIRECT_GETV     			MYRI_IO(982)
#define MX_ARM_TIMER     			MYRI_IO(985)
#define MX_WAIT_FOR_RECOVERY			MYRI_IO(986)
#define MX_SET_NIC_REPLY_INFO			MYRI_IO(987)
#define MX_SET_PEER_NAME			MYRI_IO(991)
#define MX_SET_DISPERSION	       		MYRI_IO(992)
#define MX_GET_PEER_HASH			MYRI_IO(993)
#define MX_GET_PEER_HASH_SIZE			MYRI_IO(994)
#define MX_GET_DISPERSION			MYRI_IO(995)
#define MX_IOCTL_ND_COMPLETE_OVERLAPPED		MYRI_IO(996)
#define MX_SHMEM_EN				MYRI_IO(997)
#undef MYRI_LAST_IOCTL
#define MYRI_LAST_IOCTL                         MYRI_IO(997)


/* driver/lib/fw interface
   the LSB increase for minor backwards compatible change,
   the MSB increase for incompatible change */





#define MYRI_SNF_API_MAGIC 0x400
#if MYRI_SNF_API_MAGIC >= MYRI_DRIVER_API_MAGIC
#undef MYRI_DRIVER_API_MAGIC
#define MYRI_DRIVER_API_MAGIC MYRI_SNF_API_MAGIC
#else
#error MYRI_SNF_API_MAGIC is obsolete
#endif

/* SNF gets 200 - 249 */
#define MYRI_SNF_SET_ENDPOINT_TX                MYRI_IO(200)
#define MYRI_SNF_SET_ENDPOINT_RX                MYRI_IO(201)
#define MYRI_SNF_SET_ENDPOINT_RX_RING           MYRI_IO(202)
#define MYRI_SNF_SET_ENDPOINT_RX_BH             MYRI_IO(203)
#define MYRI_SNF_RX_RING_PARAMS			MYRI_IO(204)
#define MYRI_SNF_RX_START			MYRI_IO(205)
#define MYRI_SNF_RX_STOP			MYRI_IO(206)
#define MYRI_SNF_RX_NOTIFY			MYRI_IO(207)
#define MYRI_SNF_STATS				MYRI_IO(208)
#define MYRI_SNF_WAIT			        MYRI_IO(209)
#define MYRI_SNF_SOFT_RX			MYRI_IO(210)

#undef MYRI_LAST_IOCTL
#define MYRI_LAST_IOCTL				MYRI_IO(210)


#define MYRI_MAX_STR_LEN	128


/* Explicitly pad structs to 8 bytes if they contain uint64_t, 4 bytes
   for uint32_t and smaller. Also, remember to add a compile time assert
   in mal_assertions.c to make sure the struct is the
   size you think it is. */

typedef struct {
  uint32_t driver_api_magic;
  char version_str[60];
  char build_str[128];
} myri_get_version_t;

typedef struct {
  uint32_t board;
  uint32_t pad;
  uint64_t nic_id;
} myri_get_nic_id_t;

typedef struct {
  uint32_t offset;
  uint32_t len;
  uint64_t va;
  uint32_t requested_permissions;
  uint32_t pad;
} myri_mmap_t;

typedef struct {
  uint32_t board;
  uint32_t delay;
} myri_intr_coal_t;

typedef struct {
  uint32_t board;
  uint32_t count;
  uint32_t events;
  uint32_t spurious;
} myri_get_counters_irq_t;

typedef struct {
  uint32_t board;
  uint32_t num_ports;
  uint32_t send_kb_0;
  uint32_t recv_kb_0;
  uint32_t send_kb_1;
  uint32_t recv_kb_1;
} myri_get_counters_kb_t;

typedef struct {
  uint32_t board;
  uint32_t size;
  uint64_t buffer;
} myri_get_logging_t;

typedef struct {
  uint32_t board;
  uint16_t offset;
  uint16_t width; /* 1, 2 or 4 */
  uint32_t val;
} myri_pci_cfg_t;

typedef struct {
  uint32_t pid;
  char user_info[36];
  char comm[32];
} myri_endpt_opener_t;

typedef struct {
  uint32_t board;
  uint32_t endpt;
  uint32_t closed;
  myri_endpt_opener_t opener;
} myri_get_endpt_opener_t;

typedef struct {
  uint64_t recv_buffer;
  uint64_t context;
  uint32_t incoming_port;
  uint32_t recv_bytes;
  uint32_t timeout;
  uint32_t status;
#if MYRI_ENABLE_PTP
  uint64_t timestamp_ns;
#endif
} myri_raw_next_event_t;

typedef struct {
  uint64_t data_pointer;
  uint64_t route_pointer;
  uint64_t context;
  uint8_t  route_length;
  uint8_t  physical_port;
  uint16_t data_length;
  uint32_t pad;
} myri_raw_send_t;

typedef struct {
  uint32_t board_number;
  uint32_t raw_mtu;
  uint32_t raw_max_route;
  uint32_t raw_num_tx_bufs;
} myri_raw_params_t;



#if MAL_OS_UDRV
typedef struct {
  uint32_t offset;
  uint32_t len;
  uint64_t data[64/8];
} myri_udrv_doorbell_t;
#endif


#if MYRI_ENABLE_PTP
typedef struct {
  uint32_t board;
  uint32_t avail;
} myri_ptp_rx_avail_t;
typedef struct {
  uint64_t timestamp_ns;
  uint64_t msg_pointer;
  uint32_t board;
  uint16_t msg_len;
  uint16_t raw;
} myri_ptp_store_rx_msg_t;
typedef struct {
  uint64_t timestamp_ns;
  uint64_t msg_pointer;
  uint32_t msg_len;
  uint32_t timeout_ms;
  uint32_t board;
  uint32_t raw;
} myri_ptp_get_rx_msg_t;
#define MYRI_PTP_STOP  0
#define MYRI_PTP_START 1
typedef struct {
  uint32_t board;
  uint32_t start_stop;
} myri_ptp_start_stop_t;
typedef struct {
  uint64_t timestamp_ns;
  uint32_t board;
  uint16_t ptp_type;
  uint16_t seq_id;
} myri_ptp_tx_timestamp_t;
#endif


typedef struct {
  uint32_t board_number;
  uint16_t dma_read;
  uint16_t log_size;
  uint32_t count;
  uint32_t cycles;
} mx_dmabench_t;

typedef struct {
  uint32_t board_number;
  uint32_t count;
  uint64_t indexes;
  uint64_t values;
} mx_get_some_counters_t;

typedef struct {
  int32_t endpoint;
  uint32_t session_id;
} mx_set_endpt_t;

typedef struct {
  /* each of these is mmaped independantly */
  uint32_t sendq_offset;
  uint32_t sendq_len;
  uint32_t recvq_offset;
  uint32_t recvq_len;
  uint32_t eventq_offset;
  uint32_t eventq_len;

  /* The library mmaps the sram */
  uint32_t user_mmapped_sram_offset;
  uint32_t user_mmapped_sram_len;
  uint32_t user_mmapped_zreq_offset;
  uint32_t user_mmapped_zreq_len;
  uint32_t kernel_window_offset;
  uint32_t kernel_window_len;
  uint32_t flow_window_offset;
  uint32_t flow_window_len;
  uint32_t send_compcnt_offset;
  uint32_t send_compcnt_len;

  /* And uses these offsets/lens to find the queues.  Note that these
     offsets are relative to the start of user_mmapped_sram, these are
     not to be mmapped separately.
  */
  uint32_t user_reqq_offset;
  uint32_t user_dataq_offset;
  uint32_t user_reqq_len;
  uint32_t user_dataq_len;
} mx_get_copyblock_t;

typedef struct {
  uint64_t vaddr;
  uint32_t len;
#if !MAL_OS_WINDOWS
  uint32_t pad;
#else
  uint32_t pid;
#endif
} mx_reg_seg_t;

typedef struct {
  uint32_t nsegs;
  uint32_t rdma_id;
  uint32_t seqnum;
  uint32_t pad;
  uint64_t memory_context;
  mx_reg_seg_t segs;
} mx_reg_t;

typedef struct {
  uint64_t nic_id;
  uint32_t board_number;
  uint32_t pad;
} mx_nic_id_to_board_num_t;

typedef struct {
  uint32_t board_number;
  uint32_t index;
  uint64_t nic_id;
} mx_lookup_peer_t;

#define MX_PEER_FLAG_LOCAL 1
/* the 8 upper bits are used to 
   give "active status" for each port of
   the first 4 cards (above 4 cards you don't get any)
*/
#define MX_PEER_FLAG_SEEN_NB_BOARDS 4
#define MX_PEER_FLAG_SEEN_P0 0x100
#define MX_PEER_FLAG_SEEN_P1 0x200
#define MX_PEER_FLAG_SEEN_ANY 0xff00

typedef struct
{
  uint32_t mac_low32;
  uint16_t mac_high16;
  uint16_t flags;
  uint32_t type;
  char node_name[MYRI_MAX_STR_LEN];
  uint32_t mag_id;
  uint64_t gw_id;
} mx_peer_t;

typedef struct {
  uint32_t sizeof_peer_t;
  uint32_t offset_of_type;
  uint32_t offset_of_node_name;
} mx_get_peer_format_t;

typedef struct 
{
  uint32_t mac_low32;
  uint16_t mac_high16;
  uint16_t index;
  uint16_t gw;
  uint8_t pad[6];
} mx_peer_hash_t;

typedef struct {
  uint32_t board_number;
  uint32_t pad;
  uint64_t routes;
} mx_get_route_table_t;

typedef struct {
  uint64_t nic_id;
  uint64_t va;
  uint32_t len;
  uint32_t pad;
} mx_nic_id_hostname_t;

typedef struct {
  uint32_t board_number;
  uint32_t len;
  uint64_t va;
} mx_set_hostname_t;

typedef struct {
  uint32_t board_number;
  uint8_t mapper_mac[6];
  uint16_t iport;
  uint32_t map_version;
  uint32_t num_hosts;
  uint32_t network_configured;
  uint32_t routes_valid;
  uint32_t level;
  uint32_t flags;
} mx_mapper_state_t;

typedef struct {
  uint32_t timeout;
  uint32_t status;
  uint32_t mcp_wake_events;
  uint32_t pad;
} mx_wait_t;


typedef struct {
  uint64_t dst_va;
  uint64_t src_va;
  uint32_t length;
  uint32_t src_board_num;
  uint32_t src_endpt;
  uint32_t src_session;
} mx_direct_get_t;

typedef mx_reg_seg_t mx_shm_seg_t;

typedef struct {
  mx_shm_seg_t dst_segs;
  mx_shm_seg_t src_segs;
  uint32_t dst_nsegs;
  uint32_t src_nsegs;
  uint32_t length;
  uint32_t src_board_num;
  uint32_t src_endpt;
  uint32_t src_session;
} mx_direct_getv_t;

typedef struct {
  uint32_t endpt;
  uint32_t session;
} mx_wake_endpt_t;

typedef struct {
  uint32_t board_number;
  uint32_t pad;
  uint64_t buffer;
} mx_get_eeprom_string_t;

typedef struct {
  uint32_t jiffies;
  uint32_t hz;
  uint8_t  pad[64 - 2*sizeof(uint32_t)];
  myri_clksync_seq_t clksync;
  uint8_t  pad1[64 - sizeof(myri_clksync_seq_t)];
  uint32_t attach_mask;
} mx_kernel_window_t;

typedef mx_kernel_window_t myri_kwindow_t;

typedef struct {
  uint32_t flow_desc;
  uint32_t flow_seqnum;
  uint32_t flow_data;
} myri_flow_window_t;

typedef struct {
  myri_flow_window_t  rx_flow;
  uint8_t             pad[64-sizeof(myri_flow_window_t)];
  myri_flow_window_t  tx_flow;
} myri_endpt_flow_t;



typedef struct {
  uint32_t num_rings;
  uint32_t open_flags;
  uint32_t rss_flags;
  uint32_t rx_map_flags;
  uint64_t rx_data_preamble_size;
  uint64_t rx_data_ring_size; /* without preamble */
  uint64_t rx_desc_ring_size;
} myri_snf_rx_params_t;

typedef struct {
  uint32_t epid;
  uint32_t pad;
  uint64_t tx_data_ring_size;
  uint64_t tx_data_ring_offset;
  uint64_t tx_desc_ring_size;
  uint64_t tx_desc_ring_offset;

  uint64_t tx_doorbell_size;
  uint64_t tx_doorbell_offset;
  uint64_t tx_flow_size;
  uint64_t tx_flow_offset;
  uint64_t tx_compl_size;
  uint64_t tx_compl_offset;
} myri_snf_tx_params_t;

typedef struct {
  uint32_t ring_id; /* in */
  uint32_t rx_desc_ring_count;
  uint64_t rx_desc_ring_size;
  uint64_t rx_desc_ring_offset;
  uint64_t rx_data_ring_size;
  uint64_t rx_data_ring_offset;
  uint64_t rx_data_ring_preamble_size;
} myri_snf_rx_ring_params_t;

typedef struct {
  int32_t               ring_id;  /* in-out */
  myri_snf_rx_params_t  params;
  myri_clksync_nticks_t         nticks;

  uint64_t rx_data_ring_offset;
  uint64_t rx_desc_ring_offset;

  uint64_t vstate_size;
  uint64_t vstate_offset;
  uint64_t kernel_window_size;
  uint64_t kernel_window_offset;
  uint64_t rx_doorbell_size;
  uint64_t rx_doorbell_offset;
} myri_snf_rx_attach_t;

typedef struct {
  uint64_t snf_pkt_recv;
  uint64_t snf_pkt_overflow;
  uint64_t nic_pkt_overflow;
  uint64_t nic_pkt_bad;
  uint64_t nic_bytes_recv;
  uint64_t nic_pkt_send;
  uint64_t nic_bytes_send;
} myri_snf_stats_t;

#define MYRI_SOFT_RX_CSUM_PART (1 << 0)
#define MYRI_SOFT_RX_CSUM_GOOD (1 << 1)
#define MYRI_SOFT_RX_FAKE      (1 << 2)
#define MYRI_SOFT_CSUM_MASK    ((1 << 3) - 1)
#define MYRI_SOFT_RX_TCP       (1 << 3)
typedef struct {
	uint16_t pkt_len;
	uint16_t hdr_len;
	uint16_t csum;
	uint8_t flags;
	uint8_t seg_cnt;
	struct {
		uint64_t ptr;
		uint16_t len;
		uint16_t pad[3];
	} copy_desc[2];
} myri_soft_rx_t;

#endif /* _myri_io_h_ */
