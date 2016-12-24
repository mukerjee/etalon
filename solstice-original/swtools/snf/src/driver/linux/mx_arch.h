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

#ifndef _mx_arch_h_
#define _mx_arch_h_

#include "mx_linux_compat.h"

/* Name space conflict with BSD and linux queue macros */
#ifdef LIST_HEAD
#undef LIST_HEAD
#endif
#include "bsd/queue.h"
#include "mal.h"
#include "mal_int.h"
#include "mcp_config.h"
#include "mcp_global.h"
#include "mx_misc.h"

#if !defined PAGE_SHIFT || ! defined MX_VPAGE_SHIFT
#error PAGE_SHIFT and MX_VPAGE_SHIFT should be defined here
#endif
#if MX_VPAGE_SHIFT == PAGE_SHIFT
#define MX_FACTORIZED_PAGE_PIN 1
#define mx_pin_vpages mx_pin_host_pages
#define mx_unpin_vpages mx_unpin_host_pages
#endif

#define MX_PRINT(s) printk s
#define MX_WARN(s) do {                 \
  printk(MYRI_DRIVER_STR " WARN: ");    \
  printk s;                             \
} while (0)

#define MX_INFO(s) do {                 \
  printk(MYRI_DRIVER_STR " INFO: ");    \
  printk s;                             \
} while (0)

#include "mx_pci.h"

#define MX_HAS_PCIE_LINK_RESET 1
#define MX_HAS_BRIDGE_PCI_SEC_STATUS 1
#define MX_HAS_BRIDGE_ID 1
#define MX_HAS_MAP_PCI_SPACE 1
#define MX_HAS_RELAX_ORDER_DMA 1
#define MX_HAS_INTR_RECOVER 1

#define MX_IOC_MAGIC 't'

#define MX_IOC_IO   _IO   (MX_IOC_MAGIC, 1)
#define MX_IOC_IOR  _IOR  (MX_IOC_MAGIC, 2, int)
#define MX_IOC_IOW  _IOW  (MX_IOC_MAGIC, 3, int)
#define MX_IOC_IOWR _IOWR (MX_IOC_MAGIC, 4, int)

typedef struct mx_arch_instance_info
{
  struct mx_instance_state *next;
  unsigned long interrupt;      /* 1 if we are currently inside the
                                   interrupt handler, 0 otherwise,
                                   this should be a long because of
                                   test_and_set_bit */
  struct pci_dev *pci_dev;
  unsigned int irq;
  unsigned int dac_enabled;
  int mtrr;
#if LINUX_XX <= 24
  devfs_handle_t devfs_handle[2];
#endif
  atomic_t free_iommu_pages;
  int has_iommu;
  char interrupt_string[32];
  struct timer_list kwindow_timer;
  struct timer_list clock_timer;
  struct work_struct intr_work;
  int intr_pending;
  spinlock_t intr_pending_lock;
  int is_unaligned;
  int console_cap;
  unsigned aper_size;
  uint64_t aper_base;
  struct {
    struct task_struct *kagent_task;
    struct mx_lxx_delayed_work kagent_work;
    cpumask_t kagent_wq_cpumap;
    int kagent_wq_cpu_next;
  } snf;
} mx_arch_instance_info_t;

typedef struct mx_arch_endpt_info
{
  int ops_count;
  struct mm_struct *mm;
  struct file * file;
  wait_queue_head_t shm_waitq;
  wait_queue_head_t poll_waitq;
  struct semaphore shm_lock;
  int shm_exit;
  struct {
    struct mx_lxx_delayed_work tx_flush_work;
    uint64_t                   tx_flush_cnt;
  } snf;
}
mx_arch_endpt_info_t;


typedef struct mx_sync
{
  /* fields for mutex aquire/release */
  struct semaphore mutex;
  /* fields for sleep/wake */
  struct semaphore wake_sem;
  /* fields for sleep/wake */
  atomic_t wake_cnt;
  wait_queue_head_t sleep_queue;
} mx_sync_t;

typedef spinlock_t mx_spinlock_t;

struct mx_page_pin
{
  mcp_dma_addr_t dma;
  uint64_t va;
  struct page *page;
  uint8_t private;
  uint8_t relax_order;
  uint8_t huge_page_already_pinned;
};

struct mx_instance_state;
struct mx_page_pin;

void mx_unlock_priv_page(struct mx_page_pin *pin);
int mx_lock_priv_page(struct page *page, struct mx_page_pin *pin, struct vm_area_struct *vma);


static inline int
mx_arch_copyin(uaddr_t what, void *where, size_t amount)
{
  unsigned long status;

  status = copy_from_user(where, (void *)what, amount);
  if (status) {
    return EFAULT;
  }
  return 0;
}

static inline int
mx_arch_copyout(const void *what, uaddr_t where, size_t amount)
{
  unsigned long status;

  status = copy_to_user((void *)where, what, amount);  
  if (status) {
    return EFAULT;
  }
  return 0;
}

void mx_spin(uint32_t);

/* copyblock alloc/free */

struct mx_endpt_state;
struct mx_copyblock;

int
mx_optimized_alloc_copyblock(struct mx_instance_state *is,
			     struct mx_copyblock *cb,
			     int relax_order);
void
mx_optimized_free_copyblock(struct mx_instance_state *is,
			    struct mx_copyblock *cb);

int
mx_shmem_thread_handle(struct mx_endpt_state *es, uint32_t val);

#define mx_alloc_copyblock(is,cb) mx_optimized_alloc_copyblock(is,cb, 0)
#define mx_alloc_copyblock_relax_order(is,cb) mx_optimized_alloc_copyblock(is,cb, 1)

#define mx_free_copyblock(is,cb) mx_optimized_free_copyblock(is,cb)
#if !MX_KERNEL_LIB
#define MX_DISABLE_COMMON_COPYBLOCK 1
#endif

/* linux normalizations.. */

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef minor
#define minor(a) MINOR(a)
#endif

/* 64-bit byteswapping, in case we need it */
#define mx_hton_u64(x) cpu_to_be64(x)
#define mx_ntoh_u64(x) be64_to_cpu(x)

#define bzero(ptr, len) memset(ptr, 0, len); 
#define bcopy(a, b, len) memcpy(b, a, len); 

/* atomic types and operations */
#define mx_atomic_t                     atomic_t
#define mx_atomic_add(value, ptr)       atomic_add(value, ptr)
#define mx_atomic_subtract(value, ptr)  atomic_sub(value, ptr)
#define mx_atomic_read(ptr)             atomic_read(ptr)
#define mx_atomic_set(ptr, value)       atomic_set(ptr, value)

/* spinlocks */
#define mx_spin_lock(lock)    spin_lock(lock)
#define mx_spin_unlock(lock)  spin_unlock(lock)
#define mx_spin_lock_irqsave(lock, flags) spin_lock_irqsave(lock, flags)
#define mx_spin_unlock_irqrestore(lock, flags) spin_unlock_irqrestore(lock, flags)
#define mx_spin_lock_destroy(lock)


void mx_reserve_page(void *kva);
void mx_unreserve_page(void *kva);

#define mx_linux_pfn(phys) ((unsigned long)((phys)>> PAGE_SHIFT))

static inline struct page *
mx_linux_phys_to_page(uint64_t phys)
{

#if LINUX_XX >= 26 || defined pfn_to_page
  return pfn_to_page(mx_linux_pfn(phys));
#elif LINUX_XX >= 24 && !defined CONFIG_X86_PAE
  void *virt;
  virt = phys_to_virt((long)phys);
  return virt_to_page(virt);
#else
  return mem_map + mx_linux_pfn(phys);
#endif
}

#if MAL_CPU_alpha
/* alpha architecture */

/*
  - for 21264: the bit 43 is necessary, it  marks non-cacheable space.
  - for 21164: the non-cacheable bit (bit 39) is included in the base
  address of the PCI memory region.
  - for 21264: this unfortunately cannot be the case because Linux use
  a 41 bit KSEG (it would need a 48KSEG segment)
  - we always set the bit in linux >= 2.2, it will be ignored by 21164 
  harware.
*/
#define MX_LINUX_PAGE_CACHE 0
#define MX_LINUX_PAGE_NOCACHE (1L<<(43+19))

#define MX_LINUX_IOMEM2PHYS(a) \
(virt_to_phys (ioremap((a),PAGE_SIZE)))

#define MX_LINUX_PFN_MASK (PAGE_MASK & ((1UL<<41) - 1))
/* KSEG currently limited to 41 bit, altough there seems to
   be some support for 48 bit forthcoming
   on EV6 bit 1<<40 is sign extended when using
   other bit are matched again KSEG selector, 
   and should be removed from phys
*/
#define MX_LINUX_PHYS_FROM_PTE(a) \
((pte_val(a)>>(32-PAGE_SHIFT)) & MX_LINUX_PFN_MASK)
#endif


#if MAL_CPU_x86
/* i386 architecture */

#ifndef _PAGE_PWT
#define _PAGE_PWT   0x008
#endif

/* add in no write-combining too */
#define MX_LINUX_PAGE_NOCACHE _PAGE_PCD
#define MX_LINUX_PAGE_CACHE 0
#define MX_LINUX_IOMEM2PHYS(a) (a)
#define MX_LINUX_PFN_MASK (~(uint64_t)(PAGE_SIZE-1))
#define MX_LINUX_PHYS_FROM_PTE(a) (pte_val(a) & MX_LINUX_PFN_MASK)
#endif


#if MAL_CPU_x86_64
/* x86-64 architecture */

#ifndef _PAGE_PWT
#define _PAGE_PWT   0x008
#endif

/* add in no write-combining too */
#define MX_LINUX_PAGE_NOCACHE _PAGE_PCD
#define MX_LINUX_PAGE_CACHE 0
#define MX_LINUX_IOMEM2PHYS(a) (a)
#define MX_LINUX_PFN_MASK (PHYSICAL_PAGE_MASK)
#define MX_LINUX_PHYS_FROM_PTE(a) (pte_val(a) & MX_LINUX_PFN_MASK)
#endif


#if MAL_CPU_ia64
/* ia64 architecture */
#define MX_LINUX_PAGE_NOCACHE _PAGE_MA_UC
#define MX_LINUX_PAGE_CACHE 0
#define MX_LINUX_IOMEM2PHYS(a) (a)
/* somehow walking the page table seems to result in an unexpected 1
   in bit 52, so lets clean out all the bits not concern
   above 51. --nelson
   no longer a terrible ia64 2.3.99 horrible hack in this form -- loic */
#define MX_LINUX_PFN_MASK _PFN_MASK
#define MX_LINUX_PHYS_FROM_PTE(a) (pte_val(a) & MX_LINUX_PFN_MASK)
#endif


#if MAL_CPU_sparc64
/* sparc64 architecture */
#define MX_LINUX_PAGE_NOCACHE _PAGE_E
#define MX_LINUX_PAGE_CACHE _PAGE_CACHE
#define MX_LINUX_IOMEM2PHYS(a) (__pa(a))
/* Physical Address bits [40:13]      */
#define MX_LINUX_PFN_MASK  0x000001FFFFFFE000
#define MX_LINUX_PHYS_FROM_PTE(a) (pte_val(a) & MX_LINUX_PFN_MASK)
#endif


#if MAL_CPU_powerpc
#define MX_LINUX_PAGE_NOCACHE (_PAGE_NO_CACHE)
#define MX_LINUX_PAGE_CACHE _PAGE_COHERENT

/* ioremap expect phys address for all ppc subarchs */
#define MX_LINUX_IOMEM2PHYS(a) (a)
#define MX_LINUX_PFN_MASK PAGE_MASK
#define MX_LINUX_PHYS_FROM_PTE(a) (pte_val(a) & MX_LINUX_PFN_MASK)
#endif

#if MAL_CPU_powerpc64
#define MX_LINUX_PAGE_NOCACHE (_PAGE_NO_CACHE)
#define MX_LINUX_PAGE_CACHE _PAGE_COHERENT
#define MX_LINUX_IOMEM2PHYS(a) (a)
#define MX_LINUX_PFN_MASK PAGE_MASK
#define MX_LINUX_PHYS_FROM_PTE(a) \
   ((pte_val(a) >> (PTE_SHIFT - PAGE_SHIFT)) & MX_LINUX_PFN_MASK)
#endif

#if MAL_CPU_mips
#define SC1000_PCI_INBOUND_BADDR  0x800000000
#define MX_LINUX_PAGE_NOCACHE     (_CACHE_UNCACHED)
#define MX_LINUX_PAGE_CACHE       (_CACHE_MASK)
#define MX_LINUX_IOMEM2PHYS(a)    (SC1000_PCI_INBOUND_BADDR|(a))
#define MX_LINUX_PFN_MASK         PAGE_MASK
#define MX_LINUX_PHYS_FROM_PTE(a) ((a) & MX_LINUX_PFN_MASK)
#endif

#define mx_kgetpid() ((uint32_t)current->pid)
#define mx_kgettgpid() ((uint32_t)current->tgid)

#define mx_get_memory_context() (uintptr_t)(unsigned long) current->mm

extern unsigned myri_ether_rx_frags;
extern unsigned myri_ether_rx_alloc_order;
extern unsigned myri_ether_lro;
extern unsigned myri_ether_csum;
extern unsigned myri_ether_pause;
extern unsigned myri_ether_vlan_csum_fixup;
extern unsigned myri_ether_ifname_legacy;

#define MX_HAS_DIRECT_GET_SG 1

#define mx_preempt_disable preempt_disable
#define mx_preempt_enable preempt_enable
#if 0
#define mx_getnstimeofday getnstimeofday
#else
/* Emulate over gettimeofday for now */
static inline
void mx_getnstimeofday(struct timespec *ts)
{
  struct timeval tv;
  do_gettimeofday(&tv);
  ts->tv_sec = tv.tv_sec;
  ts->tv_nsec = tv.tv_usec * 1000;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
#define mx_timespec_to_ns timespec_to_ns
#else
#define NANOSECOND 1000000000L
static inline int64_t
mx_timespec_to_ns(struct timespec *ts)
{
	int64_t ret;
	ret = ts->tv_nsec;
	ret += (int64_t)NANOSECOND *  ts->tv_sec;
	return (ret);
}
#endif

#define myri_pollwake(x) wake_up(&es->arch.poll_waitq)

#endif /* _mx_arch_h_ */
