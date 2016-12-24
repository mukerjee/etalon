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
 * Copyright 2003 - 2009 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#ifndef _mal_h_
#define _mal_h_

#include "mal_auto_config.h"
#include "mal_int.h"
#include "mal_cpu.h"

#ifndef MAL_KERNEL

#include <stddef.h>
#include <errno.h>

#if MAL_OS_WINDOWS
#include <windows.h>
typedef HANDLE mal_handle_t;
#define MAL_INVALID_HANDLE INVALID_HANDLE_VALUE
#define mal_snprintf _snprintf
#else
typedef int mal_handle_t;
#define MAL_INVALID_HANDLE -1
#define mal_snprintf snprintf
#endif

#define MX__INVALID_HANDLE MAL_INVALID_HANDLE

#if MAL_OS_MACOSX || MAL_OS_WINDOWS
#define MAL_MAP_FAILED ((void *) -1L)
#else
#include <sys/mman.h>
#define MAL_MAP_FAILED MAP_FAILED /* it is defined in sys/mman.h */
#endif

MAL_FUNC(int) mal_open(int board, int endpoint, mal_handle_t *handle);
MAL_FUNC(int) mal_open_any_board(mal_handle_t *handle);
MAL_FUNC(int) mal_close(mal_handle_t handle);
MAL_FUNC(int) mal_ioctl(mal_handle_t handle, int cmd, void *buf, size_t size);
MAL_FUNC(void *) mal_mmap(void *start, size_t length, mal_handle_t handle,
			  uintptr_t offset, int read_only);
MAL_FUNC(int) mal_munmap(void *start, size_t length);

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define mal_malloc(size) malloc(size)
#define mal_free(ptr) free(ptr)
#define mal_calloc(n,size) calloc(n,size)
#define mal_strdup(ptr) strdup(ptr)
#define mal_memcpy(x,y,z) memcpy(x,y,z)

#define mal_assertion_failed(assertion, line, file)			\
  do {									\
    printf("Error: assertion: <<%s>>  failed at line %d, file %s\n",	\
	   assertion, line, file);					\
    exit(1);								\
  } while (0)

#define MAL_PRINT(a) printf a

struct mal_ifaddrs {
  /* clones from getifaddrs(), or emulation thereof */
  struct mal_ifaddrs *mal_ifa_next;  /* Next item in list, or NULL if last */
  const char         *mal_ifa_name;  /* Interface name, as per ifconfig */
  uint32_t            mal_ifa_flags;
  uint32_t            mal_ifa_ipaddr;
  uint32_t            mal_ifa_netmask;
  uint32_t            mal_ifa_bcastaddr;

  /* Mal-specific fields */
  uint32_t            mal_ifa_boardnum;
  uint64_t            mal_ifa_nicid;
  uint8_t             mal_ifa_macaddr[6];
  uint8_t             pad[2];
  uint32_t            mal_ifa_max_endpoints;
#ifdef _WIN32
  int ifindex;
#endif
};

#define MAL_IFA_FLAG_MULTICAST 0x1
#define MAL_IFA_FLAG_BROADCAST 0x2

int mal_getifaddrs(struct mal_ifaddrs **ifaddrs_o, int inet_only, int *num_ifs);
void mal_freeifaddrs(struct mal_ifaddrs *ifaddrs);

#else /* MAL_KERNEL */

#if MAL_OS_SOLARIS || MAL_OS_FREEBSD
#include "mx_arch.h"
#endif

#include "mx_malloc.h"
#if MAL_OS_LINUX
#include <linux/kernel.h>
#elif MAL_OS_MACOSX
#include <libkern/libkern.h>
#elif MAL_OS_MACOSX || MAL_OS_SOLARIS || MAL_OS_FREEBSD
#include <sys/systm.h> /* snprintf */
#include <sys/param.h>
#elif MAL_OS_UDRV
#include <stdio.h> /* snprintf */
#endif

typedef struct mx_endpt_state * mal_handle_t;
#define MAL_INVALID_HANDLE 0
#define MX__INVALID_HANDLE 0

#define mal_open(unit, endpoint, handle) myri_klib_open(handle)
#define mal_open_any_board(handle) myri_klib_open(handle)
#define mal_ioctl(handle, cmd, arg, size) myri_klib_ioctl(handle, cmd, arg, size)
#define mal_close(handle) myri_klib_close(handle)

#define mal_malloc(size) mx_kmalloc(size, MX_WAITOK)
#define mal_free(ptr) mx_kfree(ptr)
#define mal_calloc(n,size) mx_kmalloc(n*size, MX_MZERO)
#define mal_snprintf snprintf
#define mal_memcpy(x,y,z) bcopy(y,x,z)

#define mal_debug_mask myri_debug_mask

void mal_assertion_failed(const char *assertion, int line, const char *file);

#define MAL_PRINT MX_PRINT

#endif /* !MAL_KERNEL */


#define mal_open_board(board, handle) mal_open(board, -1, handle)
#define mal_open_raw(board, handle) mal_open(board, -2, handle)


#define mal_always_assert(a)				\
  do {							\
    if (!(a))						\
      mal_assertion_failed(#a, __LINE__, __FILE__);	\
  } while (0)

extern uint32_t mal_debug_mask;

#if MAL_DEBUG

#define MAL_DEBUG_BOARD_INIT	(1 << 0)
#define MAL_DEBUG_MALLOC	(1 << 1)
#define MAL_DEBUG_OPENCLOSE	(1 << 2)
#define MAL_DEBUG_SLEEP		(1 << 3)
#define MAL_DEBUG_KVA_TO_PHYS	(1 << 4)
#define MAL_DEBUG_INTR		(1 << 5)
#define MAL_DEBUG_RAW		(1 << 6)

#define MAL_DEBUG_ACTION(mask, x)	 \
  do {					 \
    if ((mask) & mal_debug_mask)         \
      x;				 \
  } while (0)

#define MAL_DEBUG_PRINT(mask, s)         \
  do {					 \
    if ((mask) & mal_debug_mask)         \
      MAL_PRINT(s);			 \
  } while (0)

#define mal_assert(a) mal_always_assert(a)
#else
#define MAL_DEBUG_ACTION(mask, x)
#define MAL_DEBUG_PRINT(mask, s)
#define mal_assert(a) do {if (0) (void) (a);} while (0) /* syntax check */
#endif


#if !defined(likely)
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && (__GNUC__ > 2 || __GNUC__ == 2 && __GNUC_MINOR__ >= 96) 
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif
#endif



struct mal_mpool;

int mal_mpool_init(char *base, size_t size, size_t elem_sz, 
		   struct mal_mpool **pool);
void mal_mpool_fini(struct mal_mpool *pool);
void *mal_mpool_alloc(struct mal_mpool *mpool);
int mal_mpool_isempty(struct mal_mpool *mpool);
void mal_mpool_free(struct mal_mpool *mpool, void *ptr);

void mal_macaddr_to_nic_id(const uint8_t macaddr[6], uint64_t *nic_idp);
void mal_nic_id_to_macaddr(uint64_t nic_id, uint8_t macaddr[6]);
MAL_FUNC(void) mal_nic_id_to_str(char *str, uint64_t nic_id, uint32_t strlen);

#define MAL_BOARD_TYPE_D 	0 /* PCI-X D or F card, 1 2Gb port */
#define MAL_BOARD_TYPE_E		1 /* PCI-X E card, 2 2Gb ports */

#define MAL_BOARD_TYPE_ZOM	2 /* PCIe ZE card, 1 10Gb port myrinet */
#define MAL_BOARD_TYPE_ZOE	3 /* PCIe ZE card, 1 10Gb port ethernet */
#define MAL_BOARD_TYPE_ZOMS	4 /* PCIe ZES card, myrinet */
#define MAL_BOARD_TYPE_ZOES	5 /* PCIe ZES card, ethernet */
#define MAL_IS_ZE_BOARD(type)	((type) >= 2)
#define MAL_PIO_SRAM_32(type)	(((type) >= 2) && (((type) < 4)))
#define MAL_IS_ETHER_BOARD(type)	((type) == MAL_BOARD_TYPE_ZOE || \
					 (type) == MAL_BOARD_TYPE_ZOES)
#define MAL_NUM_PORTS(type)	(((type) == MAL_BOARD_TYPE_E) ? 2 : 1)


#define MX_WAIT_STATUS_GOOD			0
#define MX_WAIT_PARITY_ERROR_DETECTED		1
#define MX_WAIT_PARITY_ERROR_CORRECTED		2
#define MX_WAIT_PARITY_ERROR_UNCORRECTABLE	3
#define MX_WAIT_ENDPT_ERROR			4
#define MX_WAIT_TIMEOUT_OR_INTR			5

#define MX_MAX_WAIT	0xffffffff

/* Utility */
#define MAL_ROUND_UP(x,y) (((x) + (y)-1) & ~((y)-1))
#define MAL_DIV_UP(x,y) (((x) + (y)-1) / (y))
#define MAL_ROUND_DOWN(x,y) ((x) & ~((y)-1))

#define MAL_MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAL_MAX(x,y) ((x) > (y) ? (x) : (y))
#define MAL_ABS64(v) (((v) ^ ((v) >> 63)) - ((v)>>63))

/* Board status */
enum mx_board_status {
  MX_DEAD_RECOVERABLE_SRAM_PARITY_ERROR = 10,
  MX_DEAD_SRAM_PARITY_ERROR = 11,
  MX_DEAD_WATCHDOG_TIMEOUT = 12,
  MX_DEAD_COMMAND_TIMEOUT = 13,
  MX_DEAD_ENDPOINT_CLOSE_TIMEOUT = 14,
  MX_DEAD_ROUTE_UPDATE_TIMEOUT = 15,
  MX_DEAD_PCI_PARITY_ERROR = 16,
  MX_DEAD_PCI_MASTER_ABORT = 17,
  MX_DEAD_NIC_RESET = 18
};

/* Portable version of getifaddrs for internal library (user or kernel) */
struct mal_nic_info {
  uint32_t boardnum;
  uint32_t nic_type;
  uint64_t nic_id;
  uint32_t nic_max_endpoints;
};

int  mal_get_nicinfo(struct mal_nic_info **ninfo_o, int *num_o);
void mal_free_nicinfo(struct mal_nic_info *ninfo);


#endif /* _mal_h_ */
