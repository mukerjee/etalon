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

#ifndef _mx_instance_h_
#define _mx_instance_h_

#include "mal.h"
#include "mal_io.h"
#include "mx_pin.h"
#include "mx_misc.h"
#include "mx_pci.h"
#include "mcp_requests.h"
#include "mcp_wrapper_common.h"
#include "myri_snf_io.h"

#ifndef MX_DISABLE_ETHERNET
#define MX_DISABLE_ETHERNET 0
#endif
#ifndef MX_HAS_PCIE_LINK_RESET
#define MX_HAS_PCIE_LINK_RESET 0
#endif

/* forward declaration */
typedef struct mx_instance_state mx_instance_state_t;
typedef struct mx_endpt_state mx_endpt_state_t;
typedef struct mx_waitq mx_waitq_t;
typedef struct mx_page_pin mx_page_pin_t;
typedef struct mx_mcp_dma_chunk mx_mcp_dma_chunk_t;
typedef struct mx_host_dma_win mx_host_dma_win_t;
struct mx_ether;

typedef struct mx_copyblock
{
  void *alloc_addr;
  void *addr;
  uint32_t size;
  mx_page_pin_t *pins;
  mcp_dma_addr_t *dmas;
#if MAL_OS_WINDOWS
  PMDL mdl;
#endif
} mx_copyblock_t;

typedef struct mx_huge_copyblock
{
#if MAL_OS_LINUX || MAL_OS_UDRV
  struct page **pages;
#elif MAL_OS_WINDOWS
  PMDL mdl;
#endif
  void *kva;
  uint64_t size;
  mx_page_pin_t *pins;
  mx_copyblock_t cb;
  unsigned long pinned_pages;
} mx_huge_copyblock_t;

typedef struct mx_intr
{
  uint32_t slot;
  uint16_t seqnum;
  uint32_t spurious;
  uint32_t cnt;
  mcp_intr_t *ring;
  mx_page_pin_t pin;
} mx_intr_t;

typedef struct mx_routes
{
  char *host_table;
  char *mcp_table;
  int *indexes;
  int *offsets;
  int block_size;
} mx_routes_t;

typedef int (mx_init_board_t)(mx_instance_state_t *is);
typedef int (mx_map_board_t)(mx_instance_state_t *is);
typedef void (mx_unmap_board_t)(mx_instance_state_t *is);
typedef void (mx_claim_interrupt_t)(mx_instance_state_t *is);
typedef int (mx_read_irq_level_t)(mx_instance_state_t *is);
typedef void (mx_enable_interrupt_t)(mx_instance_state_t *is);
typedef void (mx_disable_interrupt_t)(mx_instance_state_t *is);
typedef void (mx_park_board_t)(mx_instance_state_t *is);
typedef int (mx_detect_parity_t)(mx_instance_state_t *is);
typedef void (mx_get_freq_t)(mx_instance_state_t *is);
typedef void (mx_write_kreq_t)(mx_instance_state_t *is, mcp_kreq_t *kreq);
typedef uint32_t (mx_get_rtc_t)(mx_instance_state_t *is);

typedef struct mx_board_ops {
  mx_init_board_t 		*init;
  mx_map_board_t 		*map;
  mx_unmap_board_t 		*unmap;
  mx_claim_interrupt_t 		*claim_interrupt;
  mx_read_irq_level_t		*read_irq_level;
  mx_enable_interrupt_t 	*enable_interrupt;
  mx_disable_interrupt_t 	*disable_interrupt;
  mx_park_board_t		*park;
  mx_detect_parity_t		*detect_parity_error;
  mx_get_freq_t		        *get_freq;
  mx_write_kreq_t		*write_kreq;
  mx_get_rtc_t			*get_rtc;
} mx_board_ops_t;

extern mx_board_ops_t mx_lx_ops;
extern mx_board_ops_t mx_lz_ops;
void mx_lx_init_board_ops(void);
void mx_lz_init_board_ops(void);
void mx_lz_read_mac(mx_instance_state_t *is, char mac[18]);
int mx_strcasecmp(const char *a, const char *b);

#define MX_ADDRS_PER_VPAGE  (MX_VPAGE_SIZE / sizeof(mcp_dma_addr_t))
#define MX_ADDRS_PER_VPAGE_SHIFT  (MX_VPAGE_SHIFT - MX_DMA_ADDR_SHIFT)

#define MX_NUM_DIRECT_DMA  4  /* XXXX move me elsewhere!!!! */
#define MX_MAX_MAPPER_TYPES 5


typedef struct mx_mmu
{
  struct mx_mmu* next;
  uint64_t key;
  uint32_t mcp_offset;

  mcp_dma_addr_t *meta_addr[MX_MMU_BLOCK];
} mx_mmu_t;


/* An mx_dma_table contains one page of DMA addresses.
   Lengths for each DMA are implicitly assumed to be VPAGE size.
   The MCP accesses this structure, so it must be contiguous in DMA space.  
   The table can be a pte table or a higher-level table */

struct mx_dma_table
{
  mx_page_pin_t pin;
  mcp_dma_addr_t *desc;		/* pointer to DMA page */
  union {
    mx_page_pin_t *pins; /* DMA pin info for data pages */
    struct mx_dma_table **tables; /* tables of inferor lvel  */
  } u;
  int nb_entries;
  int nb_subtables;
  int log_level; /* 0=> table of data pages , n*MX_ADDRS_PER_VPAGE_SHIFT => n-higher-level table */
};

typedef struct mx_dma_table mx_dma_table_t;

/* An mx_host_dma_win_t is a container for one rdma window,
   it contains the page table associated with this window. */

struct mx_host_dma_win
{
  uint32_t flags;
  uint32_t length;
  uint32_t seqnum;
  struct mx_dma_table table;
};

typedef struct myri_raw_tx_buf
{
  STAILQ_ENTRY(myri_raw_tx_buf) next;
  mcp_dma_addr_t dma;
  void *buf;
  uint32_t id;
  uint64_t context;
} myri_raw_tx_buf_t;

STAILQ_HEAD(myri_raw_tx_bufq, myri_raw_tx_buf);


enum myri_snf_rx_state { MYRI_SNF_RX_S_FREE = 0,
                         MYRI_SNF_RX_S_INIT = 1,
                         MYRI_SNF_RX_S_STARTED = 2,
                         MYRI_SNF_RX_S_STOPPED = 3 };

enum myri_snf_es_type { MYRI_SNF_ES_FREE = 0,
                        MYRI_SNF_ES_RX = 1,
                        MYRI_SNF_ES_RX_RING = 2,
                        MYRI_SNF_ES_TX = 3 };

struct snf__rx;

struct myri_snf_kagent_params {
  enum myri_snf_kagent_state {
    MYRI_SNF_KAGENT_DISABLED,
    MYRI_SNF_KAGENT_STARTING,
    MYRI_SNF_KAGENT_STARTED,
    MYRI_SNF_KAGENT_STOPPING,
  } state;
  struct snf__params p;
  mx_sync_t         sync;
  struct snf__rx   *rx;
  mx_endpt_state_t *es;
};

typedef struct {
  mx_instance_state_t   *is;
  mx_sync_t              sync;
  uint32_t               attach_mask;
  int                    refcount;
  enum myri_snf_rx_state state;
  myri_snf_rx_params_t   params;

  struct snf__rx_state  *rx_s;
  unsigned               rx_s_length;
  uint64_t              *rx_doorbell_be64;

  mx_huge_copyblock_t desc_cb;
  mx_huge_copyblock_t data_cb;

  /* copyblocks for each receive ring */
  struct myri_snf_rx_ring_p {
    mx_huge_copyblock_t       hcb;
    void                     *kva;
    myri_snf_rx_ring_params_t p;
  } rx_ring_cbs[MYRI_SNF_MAX_RINGS];
  uintptr_t rx_ring_hcb_size; /* per ring cb size */
  uintptr_t rx_ring_data_size;
  uintptr_t rx_ring_desc_size;
  uintptr_t rx_ring_desc_count;
  int       rx_ring_node_id;

  uint32_t  owner_pid;
  int       num_rings_attached;

  mx_endpt_state_t *ring_map_ep[MYRI_SNF_MAX_RINGS];

  /* RX kagent */
  struct myri_snf_kagent_params kagent;

  /* RX interrupts */
  mx_endpt_state_t *rx_intr_es;
  int               rx_intr_requested;
  mx_spinlock_t     rx_intr_lock;
} myri_snf_rx_state_t;

void     myri_snf_rx_teardown(mx_endpt_state_t *es);
uint32_t myri_snf_intr_req(mx_endpt_state_t *es, uint32_t intr_flags);
int      myri_snf_intr_tx_try_flush(mx_endpt_state_t *es);
int      myri_snf_find_node_id(mx_endpt_state_t *es);

/* OS-specific */
void myri_snf_handle_tx_intr(mx_endpt_state_t *es);

void myri_snf_rx_wake(myri_snf_rx_state_t *snf, mx_endpt_state_t *es);
#if MYRI_SNF_KAGENT
int myri_snf_kagent_init(myri_snf_rx_state_t *snf);
int myri_snf_kagent_fini(myri_snf_rx_state_t *snf);
void myri_snf_kagent_body(myri_snf_rx_state_t *snf);
void myri_snf_rx_kagent_progress(myri_snf_rx_state_t *snf, int work_queue);
int myri_snf_recv_agent_process(struct snf__rx *rx, int work_queue);
#endif

/* HW clock functionality */
int  myri_clock_init(mx_instance_state_t *is);
void myri_clock_destroy(mx_instance_state_t *is);
int  myri_clock_sync_now(mx_instance_state_t *is);

#if MYRI_ENABLE_PTP
enum myri_ptp_state {
  MYRI_PTP_STOPPED  = -1,
  MYRI_PTP_STARTING = 0,
  MYRI_PTP_RUNNING  = 1
};
#endif

#define MX_IS_DEAD        (1 << 0)
#define MX_IS_LOGGING     (1 << 1)
#define MX_HOSTNAME_SET   (1 << 2)
#define MX_PARITY_RECOVERY (1 << 3)

#define MYRI_WAIT_RX_EVENT  1
#define MYRI_WAIT_TX_EVENT  2

struct mx_instance_state
{
  unsigned int id;            /* unit number.         */
  unsigned int flags;         /* Flags, defined above */
  mx_board_ops_t board_ops;

  /*************
   * LANai access fields
   *************/

  /* Record of lanai mappings */
    
  struct
  {
    /* This pointer is opaque to ensure that the special registers
       are referenced only through the pio.h interface. */
    void *special_regs;
    volatile uint32_t *control_regs;
    volatile uint8_t *sram;
#if MX_DMA_DBG
    volatile uint32_t *dma_dbg_bitmap;
    volatile uint32_t dma_dbg_lowest_addr;
#endif
    /* The globals are not a structure to insure that they are
       referenced only throught the pio.h interface. */
    volatile void *globals; /* Set iff MCP running. */
    uint32_t running;
    char *eeprom_strings;
    char *product_code;
    char *part_number;
    uint32_t serial;
  } lanai;

  /* Pointer to LANai interface card, mapped into kernel memory. */

  uint32_t board_span;
  uint32_t sram_size;
  uint32_t specials_size;
  uint32_t using_msi;
  uint32_t msi_enabled;
  uint32_t cpu_freq;
  uint32_t pci_freq;
  mx_arch_instance_info_t arch; /* Architecture-specific field. */
  mx_sync_t sync;
  mx_endpt_state_t **es;  
  int es_count;
  char is_name_default[8];
  const char *is_name;
  uint8_t mac_addr[8];
  char *mac_addr_string;
  struct {
    mx_page_pin_t pin;
    char *addr;
    uint32_t index;
  } endpt_desc;
  mx_intr_t intr;
  struct {
    mcp_kreq_t kreq;
    mx_spinlock_t spinlock;
  } cmd;
  mcp_kreq_t *kreqq;
  int kreqq_submitted;
  int kreqq_max_index;
  mx_spinlock_t kreqq_spinlock;
  struct {
    mx_sync_t sync;
    uint32_t size;
  } logging;
  mx_sync_t init_sync;
  mx_atomic_t ref_count;
  struct {
    mx_endpt_state_t *es;
    int use_count;
    mx_spinlock_t spinlock;
    mx_sync_t sync;
    int wakeup_needed;
    myri_raw_tx_buf_t *tx_bufs;
    struct myri_raw_tx_bufq tx_bufq;
    mx_copyblock_t tx_cb;
    uint32_t tx_req;
    uint32_t tx_done;
    mx_copyblock_t rx_cb;
    uint32_t rx_cnt;
    mcp_raw_desc_t *descs;
    mx_page_pin_t descs_pin;
    uint32_t desc_cnt;
  } raw;
  uint32_t num_ports;
  uint8_t pci_rev;
  uint16_t pci_devid;
  uint32_t bar0_low;
  uint32_t bar0_high;
  uint32_t bar2_low;
  uint32_t bar2_high;
  uint32_t msi_addr_low;
  uint32_t msi_addr_high;
  uint16_t msi_data;
  uint16_t msi_ctrl;
  uint16_t exp_devctl;
  uint32_t ze_print_pos;
  int board_type;
  uint32_t mcp_print_len;
  uint32_t mcp_print_addr;
  uint32_t mcp_print_idx;
  char *mcp_print_buffer;
  uint32_t old_lanai_uptime;
  struct mx_ether *ether;
  int ether_is_open;
  struct {
    mx_sync_t wait_sync;
    uint32_t cycles;
    uint32_t count;
    int busy;
  } dmabench;
  struct {
    mx_mmu_t *host_hash[MX_MMU_HASH];
    mx_mmu_t *host_table;
    mx_mmu_t *free;
    uint32_t *mcp_hash;
    mcp_mmu_t *mcp_table;
  } mmu;
  int parity_errors_detected;
  int parity_errors_corrected;
  uintptr_t parity_critical_start;
  uintptr_t parity_critical_end;
  mx_sync_t ready_for_recovery_sync;
  struct {
    int command_tweaked;
    uint16_t command_offset;
    uint16_t command_orig_val;
  } pci_cap;
  uint32_t link_state;
  struct {
    int reason;
    int arg;
  } saved_state;
  struct {
    mx_page_pin_t pin;
    char *addr;
  } tx_done;
  struct {
    mx_page_pin_t pin;
    char *addr;
  } sysbox;
  struct  {
    mx_page_pin_t pin;
    char *addr;
    uint32_t *mcp;
  } counters;
  struct {
    mx_page_pin_t pin;
    char *addr;
  } bogus;
  mx_kernel_window_t *kernel_window;
  uint32_t *dma_dbg_bitmap_host;
  int manual_dispersion;
  uint32_t crc32_table[16];
  myri_snf_stats_t   snf_stats;
  myri_snf_stats_t   snf_stats_prev;
  myri_snf_rx_state_t   snf;
  void              *snf_rx_doorbell;
  /* NIC and host clock synchronization */
  mx_spinlock_t           clk_spinlock;
  int                     clk_initialized;
  uint32_t                clk_db_offset;
  int                     clk_hwclock_seqno;
  int                     clk_sync_init_cnt;
  uint64_t                clk_sync_init_nsecs;
  mcp_hwclock_update_t   *clk_update;
  myri_clksync_nticks_t           clk_nticks;
  myri_clksync_params_t      clkp_latest;
#if MYRI_ENABLE_PTP
  enum myri_ptp_state     ptp_state;
  mx_spinlock_t           ptp_state_lock;
  struct myri_ptp        *ptp;
#endif
};

#define MX_RDMA_WIN_PER_PAGE (MX_VPAGE_SIZE/sizeof(mcp_rdma_win_t))

#define MX_ES_CLOSING	(1 << 0)
#define MX_ES_RECOVERY	(1 << 1)

enum myri_endpt_type { 
  MYRI_ES_MX = 0,
  MYRI_ES_RAW,
  MYRI_ES_SNF_RX,
};

struct mx_endpt_state
{
  mx_instance_state_t *is;
  uint32_t endpt;
  enum myri_endpt_type es_type;
  uint32_t flags;
  uint32_t ref_count;  /* access protected by sync */
  mx_sync_t sync;

  /* copyblocks */
  mx_copyblock_t sendq;
  mx_huge_copyblock_t recvq;
  mx_copyblock_t eventq;
  mx_copyblock_t user_mmapped_sram;  /* sram, not kernel mem */
  mx_copyblock_t user_mmapped_zreq;  /* mmio, not kernel mem */


  /* waiting */
  mx_sync_t wait_sync;
  mx_sync_t app_wait_sync;
  mx_atomic_t no_mcp_req_wakeups;
  unsigned progression_timeout;
  int app_waiting;
  mx_sync_t close_sync;
  uint32_t num_waiters;
  mx_sync_t recovery_sync;

  /* MD handle */
  mx_arch_endpt_info_t arch;

  /* Memory locked by large send/recv indexed by handle*/
  struct {
    mx_host_dma_win_t *win;
    uint16_t seqno;
  } *host_rdma_windows;

  /* where to write the vpages containing user-area dma mappings */
  mcp_rdma_win_t *mcp_rdma_windows;

  myri_endpt_flow_t *flow_window;
  mx_page_pin_t flow_window_pin;	
  uint32_t privileged;
  uint32_t is_kernel;
  uint32_t euid;
  myri_endpt_opener_t opener;
  int parity_errors_detected;
  int parity_errors_corrected;
  int endpt_error;
  uint32_t session_id;

  /* If endpoint is pinned to a cpu, keep track of its cpuid and nodeid, if
   * applicable */
  int cpu_id;
  int node_id;

  struct {
    enum myri_snf_es_type es_type;
    TAILQ_ENTRY(mx_endpt_state) link;
    int           ring_id;
    uint32_t      flags;
    mx_spinlock_t tx_lock;
    int           tx_intr_requested;
    int           rx_intr_requested;
    struct snf__tx_shared_params tx_sp;
  } snf;
};

int mx_init_driver(void);
int mx_finalize_driver(void);

int  mx_instance_init (mx_instance_state_t *is, int32_t unit);
int  mx_instance_finalize (mx_instance_state_t *is);
void mx_lanai_print(mx_instance_state_t *is, int idx);
int mx_load_mcp(mx_instance_state_t *is, void * img, int limit, int *actual_len);


void *mx_map_pci_space (mx_instance_state_t * is, int bar, uint32_t offset, 
		       uint32_t len);

void *mx_map_io_space (mx_instance_state_t * is, uint32_t offset, 
		       uint32_t len);
void  mx_unmap_io_space (mx_instance_state_t * is,
			 uint32_t len, void *kaddr);

void mx_dump_interrupt_ring(mx_instance_state_t *is);


extern mx_instance_state_t **mx_instances;
extern uint32_t myri_max_instance;
extern uint32_t mx_num_instances;
extern uint32_t mx_mapper_mask;
extern uint32_t mx_mapper_running_map;
extern uint32_t mx_mapper_level_mask;
extern uint32_t mx_mapper_verbose;
extern int mx_mapper_msgbuf_size;
extern int mx_mapper_mapbuf_size;
extern uint32_t myri_mx_small_message_threshold;
extern uint32_t myri_mx_medium_message_threshold;
extern uint32_t myri_security_disabled;
extern int mx_kernel_mapper;
extern uint32_t myri_z_loopback;
extern uint32_t myri_ethernet_bitmap;
extern int mx_has_xm;
extern char * myri_mac;

#if MAL_OS_MACOSX || MAL_OS_SOLARIS
#define MX_MAX_INSTANCE_DEFAULT 4
#else
#define MX_MAX_INSTANCE_DEFAULT 32
#endif

extern uint32_t myri_mx_max_nodes;
extern uint32_t myri_max_endpoints;
extern uint32_t myri_recvq_vpage_cnt;
extern uint32_t myri_eventq_vpage_cnt;

#define MX_MAX_ENDPOINTS_DEFAULT 16

extern uint32_t myri_intr_coal_delay;

extern mx_atomic_t myri_max_user_pinned_pages;
extern uint32_t myri_mx_max_send_handles;
extern uint32_t mx_max_pull_handles;
extern uint32_t mx_max_push_handles;
extern uint32_t myri_mx_max_rdma_windows;
extern uint32_t mx_cacheline_size;
extern uint32_t myri_override_e_to_f;
extern uint32_t myri_parity_recovery;
extern uint32_t myri_recover_from_all_errors;
extern uint32_t myri_pcie_down;
extern uint32_t myri_pcie_down_on_error;
extern uint32_t myri_mx_max_host_queries;
extern uint32_t myri_dma_pfn_max;
extern uint32_t myri_force;
extern uint32_t myri_jtag;
extern uint32_t myri_mx_max_macs;
extern uint32_t myri_irq_sync;
extern uint32_t myri_msi_level;
extern uint32_t myri_ether_pause;
extern uint32_t myri_clksync_period;
extern uint32_t myri_clksync_error;
extern uint32_t myri_clksync_error_count;
extern uint32_t myri_clksync_verbose;
extern uint32_t myri_snf_rings;
extern uint32_t myri_snf_flags;

extern mx_spinlock_t mx_lanai_print_spinlock;

extern char mx_default_hostname[MYRI_MAX_STR_LEN];

/* sleeping and waking up */

#define MX_SLEEP_INTR    1
#define MX_SLEEP_NOINTR  2

/* for the watchdog to properly detect parity error (rather than such
   error ending up as some MX_DEAD_xxx_TIMEOUT), the final timeout
   should be:
   final-timeout >= (min-desired-timeout + 2*Watchdog)
   in addition to sram parity error, this avoid confusing root cause
   and secondary effects for any HW check done in the watchdog_body
*/

#define MX_SMALL_WAIT    100
#define MX_CLOSE_WAIT	 1000*30 /* 30 seconds */
#define MX_LOGGING_WAIT  MX_MAX_WAIT

#if MAL_OS_UDRV
#define MCP_INIT_TIMEOUT	(mx_lxgdb ? 600*1000 : 12*1000)
#define MCP_COMMAND_TIMEOUT	(mx_lxgdb ? 600*1000 : 10)
#define MX_WATCHDOG_TIMEOUT	(mx_lxgdb ? 10 : 5)
#define MX_LXGDB		mx_lxgdb
#else
/* FIXME: do we need 10s timeout ? */
#define MCP_INIT_TIMEOUT	12*1000
#define MCP_COMMAND_TIMEOUT	10
#define MX_WATCHDOG_TIMEOUT	5
#define MX_LXGDB		0
#endif


void mx_wake(mx_sync_t *);
int mx_wake_once(mx_sync_t *); /* returns 1 if notify happened */
int mx_sleep(mx_sync_t *, int, int flags);
mx_waitq_t *mx_new_waitq(void);
void mx_free_waitq(mx_waitq_t *);

/* syncs.. need to move elsewhere */

void mx_sync_init (mx_sync_t *s, mx_instance_state_t *is, int unique, 
		   char *string);
void mx_sync_reset (mx_sync_t *s);
void mx_sync_destroy (mx_sync_t *s);

#ifndef mx_spin_lock_destroy
void mx_spin_lock_destroy(mx_spinlock_t *s);
#endif

void mx_spin_lock_init(mx_spinlock_t *s, mx_instance_state_t *is, int unique, 
		       char *string);

#ifndef mx_mutex_enter
void mx_mutex_enter (mx_sync_t *s);
int  mx_mutex_try_enter (mx_sync_t *s); /* returns 0 if successful */
void mx_mutex_exit (mx_sync_t *s);
#endif

extern mx_sync_t mx_global_mutex;

/* open/close/ioctl .. need to move elsewhere */


int myri_snf_ioctl(mx_endpt_state_t *es, uint32_t cmd, const uaddr_t in);
int mx_common_ioctl(mx_endpt_state_t *es, uint32_t cmd, const uaddr_t in);

int mx_endptless_ioctl(uint32_t cmd, const uaddr_t in,
		       uint32_t privileged, uint32_t is_kernel);

int mx_common_open(int32_t unit, int32_t endpoint, mx_endpt_state_t *es, 
                   enum myri_endpt_type es_type);
void mx_common_close (mx_endpt_state_t *es);

void mx_unpin_page(mx_instance_state_t *, mx_page_pin_t *, int);
void mx_unpin_vpages(mx_instance_state_t *, mx_page_pin_t *, int, int);
int mx_pin_page(mx_instance_state_t *, mx_page_pin_t *, int, uint64_t);
int mx_pin_vpages(mx_instance_state_t *, mx_page_pin_t *, mcp_dma_addr_t *, int, int, uint64_t);
mx_page_pin_t *mx_find_pin(mx_endpt_state_t *es,  
			   int (*testfp)(mx_page_pin_t *pin, void *arg), void *arg);

/* PCI accessor functions */
int mx_read_pci_config_8 (mx_instance_state_t *is,
			  uint32_t offset,
			  uint8_t *val);

int mx_read_pci_config_16 (mx_instance_state_t *is,
			   uint32_t offset,
			   uint16_t *val);

int mx_read_pci_config_32 (mx_instance_state_t *is,
			   uint32_t offset,
			   uint32_t *val);

int mx_write_pci_config_8 (mx_instance_state_t *is,
			   uint32_t offset,
			   uint8_t val);

int mx_write_pci_config_16 (mx_instance_state_t *is,
			    uint32_t offset,
			    uint16_t val);

int mx_write_pci_config_32 (mx_instance_state_t *is,
			    uint32_t offset,
			    uint32_t val);


#if MAL_OS_UDRV
void mx_write_lanai_isr(struct mx_instance_state *is, uint32_t val);
void mx_write_lanai_ctrl_bit(struct mx_instance_state *is, int val);
#else
#define mx_write_lanai_isr(is,val) mx_write_lanai_special_reg_u32(is,lx.ISR, val)
#define mx_write_lanai_ctrl_bit(is,bit) mx_write_pci_config_32(is, 0x40, 1U << (bit))
/* or equivalent: is->lanai.control_regs[0x10] = htonl(1 << MX_LX_CTRL(bit)))) */
#endif

#define MX_MEM_HOSTMEM 0
#define MX_MEM_SRAM 1
#define MX_MEM_CONTROL 2
#define MX_MEM_SPECIAL 3
#define MX_MEM_HOSTMEM_UNMAPPED 4

int mx_mmap_off_to_kva(mx_endpt_state_t *, unsigned long, void **, int *,
		       mx_page_pin_t **);
int myri_snf_mmap_off_to_kva(mx_endpt_state_t *es, unsigned long req, 
                             unsigned long *off, void **kva,
		             int *mem_type, mx_page_pin_t **pin);


int mx_mcp_command(mx_instance_state_t *, uint8_t, uint32_t, uint32_t, 
		   uint32_t, uint32_t *);
void mx_mcp_command_done(mx_instance_state_t *, mcp_cmd_status_t, uint32_t);
  
int mx_common_interrupt(mx_instance_state_t *is);


/* memory registration related functionality */
void mx_free_dma_win (mx_instance_state_t *is, mx_host_dma_win_t *hc);
mx_host_dma_win_t *mx_new_dma_win(mx_instance_state_t *is, int nsegs);
void mx_mmu_init(mx_instance_state_t *is);
int mx_mmu_register(mx_endpt_state_t *es, mx_reg_seg_t *usegs, uint32_t num_usegs, uintptr_t memory_context);
int mx_mmu_deregister(mx_endpt_state_t *es, uint64_t va, uint32_t vpages, uint32_t flags);

static inline int
mx_is_dead(mx_instance_state_t *is)
{
  return (is->flags & (MX_IS_DEAD | MX_PARITY_RECOVERY));
}
#define mx_digit(c) (((c) >= '0' && (c) <= '9') ? ((c) - '0') : \
                 (((c) >= 'A' && (c) <= 'F') ? (10 + (c) - 'A') : \
                  (((c) >= 'a' && (c) <= 'f') ? (10 + (c) - 'a') : -1)))

int mx_alloc_dma_page(mx_instance_state_t *is, char **addr, mx_page_pin_t *pin);

void mx_free_dma_page(mx_instance_state_t *is, char **addr, mx_page_pin_t *pin);
int mx_alloc_zeroed_dma_page(mx_instance_state_t *is, char **addr, mx_page_pin_t *pin);
#if MAL_OS_LINUX
int mx_alloc_zeroed_dma_page_node(mx_instance_state_t *is, char **addr, mx_page_pin_t *pin, int node_id);
#else
#define mx_alloc_zeroed_dma_page_node mx_alloc_zeroed_dma_page
#endif
int mx_alloc_zeroed_dma_page_relax_order(mx_instance_state_t *is, char **addr, mx_page_pin_t *pin);

#ifdef MX_HAS_RELAX_ORDER_DMA
int mx_alloc_dma_page_relax_order(mx_instance_state_t *is, char **addr, mx_page_pin_t *pin);
#else
#define mx_alloc_dma_page_relax_order mx_alloc_dma_page
#define mx_alloc_copyblock_relax_order mx_alloc_copyblock
#endif

int mx_rand(void);
void mx_watchdog_body(void);


int mx_dma_map_copyblock(mx_endpt_state_t *es, mx_copyblock_t *cb, uint32_t offset);

int mx_common_alloc_copyblock(mx_instance_state_t *is, mx_copyblock_t *cb);
void mx_common_free_copyblock(mx_instance_state_t *is, mx_copyblock_t *cb);
void mx_parse_mcp_error(mx_instance_state_t *is);
#if MAL_OS_LINUX
void *mx_map_huge_copyblock(mx_huge_copyblock_t *hcb);
void mx_unmap_huge_copyblock(mx_huge_copyblock_t *hcb);
#else
#define mx_map_huge_copyblock(hcb) NULL
#define mx_unmap_huge_copyblock(hcb) do { } while (0)
#endif


/* copyin and out */
static inline int
mx_copyin(uaddr_t what, void *where, size_t amount, uint32_t is_kernel)
{
  if (is_kernel) {
    bcopy((void *)(uintptr_t)what, where, amount);
    return 0;
  } else
    return mx_arch_copyin(what, where, amount);
}

static inline int
mx_copyout(void *what, uaddr_t where, size_t amount, uint32_t is_kernel)
{
  if (is_kernel) {
    bcopy(what, (void *)(uintptr_t)where, amount);
    return 0;
  } else
    return mx_arch_copyout(what, where, amount);
}

int mx_instance_status_string(int unit, char **str, int *len);

void mx_set_default_hostname(void);

#define mx_start_mapper(is)
#define mx_stop_mapper(is)

#if MAL_OS_LINUX
void myri_update_numa_config(mx_endpt_state_t *es);
#else
static inline void myri_update_numa_config(mx_endpt_state_t *es)
{
  es->cpu_id = -1;
  es->node_id = -1;
}
#endif


mx_endpt_state_t *mx_get_endpoint(mx_instance_state_t *is, int endpt);
void mx_put_endpoint(mx_endpt_state_t *es);
int mx_direct_get(mx_endpt_state_t *dst_es, mx_shm_seg_t *dst_segs, uint32_t dst_nsegs,
		  mx_endpt_state_t *src_es, mx_shm_seg_t *src_segs, uint32_t src_nsegs,
		  uint32_t length);
mx_instance_state_t *mx_get_instance(uint32_t unit);
void mx_release_instance(mx_instance_state_t *is);
void mx_mark_board_dead(mx_instance_state_t *is, int reason, int arg);
void mx_parse_eeprom_strings(mx_instance_state_t *is);

int mx_direct_get_common(mx_shm_seg_t *dst_segs, uint32_t dst_nsegs,
			 void * src_space, mx_shm_seg_t *src_segs, uint32_t src_nsegs,
			 uint32_t length);

/* OS specific callback for direct get, copying from another process
 * user-space to current process user-space.
 */
int mx_arch_copy_user_to_user(uaddr_t udst, uaddr_t usrc, void * src_space,
			      uint32_t len);


int mx_pcie_link_reset(mx_instance_state_t *is);
int mx_intr_recover(mx_instance_state_t *is);
uint16_t mx_bridge_pci_sec_status(mx_instance_state_t *is);
uint32_t mx_bridge_id(mx_instance_state_t *is);
int mx_find_capability(mx_instance_state_t *is, unsigned cap_id);

int myri_select_mcp(int board_type, const unsigned char **mcp,
		    int *len, unsigned int *parity_critical_start,
		    unsigned int *parity_critical_end);
int myri_counters(int board_type, const char ***counters, uint32_t *count);
uint32_t myri_get_counter(mx_instance_state_t *is, mcp_counter_name_t counter);

int
mx_alloc_huge_copyblock(mx_instance_state_t *is, mx_huge_copyblock_t *hcb,
			unsigned long lib_offset, int node_id);

void
mx_free_huge_copyblock(mx_instance_state_t *is, mx_huge_copyblock_t *hcb);

void myri_snf_get_stats(mx_instance_state_t *is, myri_snf_stats_t *stats_o);
void myri_snf_clear_stats(mx_instance_state_t *is, int reset);
void myri_poll_prep(mx_endpt_state_t *es);
int myri_poll_wait(mx_endpt_state_t *es, int timeout_ms, int flags);
int myri_poll_events_pending(mx_endpt_state_t *es);


#endif /* _mx_instance_h_ */
