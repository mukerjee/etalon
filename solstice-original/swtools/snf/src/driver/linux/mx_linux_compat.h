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

/* modifications for MX kernel lib made by
 * Brice.Goglin@ens-lyon.org (LIP/INRIA/ENS-Lyon) */

#ifndef _mx_linux_compat_h_
#define _mx_linux_compat_h_

#include "mal_checks.h"
#include <linux/version.h>
// #ifndef AUTOCONF_INCLUDED
/* linux/config.h warns in 2.6.19 and does not exist after 2.6.20, do not include it anymore */
/* linux/autoconf.h is automatically included by kbuild since 2.6.15 anyway */
// #include <linux/config.h>
// #endif
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/pci.h>
#include <linux/utsname.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/netdevice.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>
#ifdef CONFIG_MTRR
#include <asm/mtrr.h>
#endif
#include <linux/mm.h>
// #include <linux/smp_lock.h>
#include <linux/smp.h>

#ifndef UTS_RELEASE
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)
#include <generated/utsrelease.h>
#else
#include <linux/utsrelease.h>
#endif
#endif

#include "mal.h"

#include <linux/pagemap.h>
#ifdef HAVE_LINUX_COMPILE_H
#include <linux/compile.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) && !defined CONFIG_DEVFS_FS
#define devfs_mk_cdev(dev, mode, fmt)
#define devfs_remove(dev,i)
#else
#include <linux/devfs_fs_kernel.h>
#endif

/*
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#warning "This driver does not support Linux 2.2.x or earlier."
#error "Please use Linux 2.4.x or 2.6.x."
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#define LINUX_XX 24
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,7,0)
#define LINUX_XX 26
#else
#error "Unsupported Linux kernel release."
#endif
*/
#define LINUX_XX 300

/* PAGE_OFFSET should be the first physical page of memory, that
   should works even if memory does not begin at physical address 0 */
#define MX_LINUX_PFN_ZERO (__pa((void*)PAGE_OFFSET) / PAGE_SIZE)
#if LINUX_XX >= 26
#define MX_LINUX_PFN_MAX (MX_LINUX_PFN_ZERO + num_physpages)
#else
#define MX_LINUX_PFN_MAX (MX_LINUX_PFN_ZERO + max_mapnr)
#endif
#define MX_LINUX_KERNEL_PFN_MAX (__pa((void*)high_memory)/PAGE_SIZE)


#if defined(HAVE_MMAP_UP_WRITE)
#define mx_mmap_up_write mmap_up_write
#define mx_mmap_down_write mmap_down_write
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,4,3)
#define mx_mmap_up_write(a) up(&(a)->mmap_sem)
#define mx_mmap_down_write(a) down(&(a)->mmap_sem)
#elif defined __alpha__ && LINUX_VERSION_CODE < KERNEL_VERSION(2,4,7)
/* __down_write and __up_write not exported */
/* not getting the semaphore is still safe because we have the page_table_lock
   and the kernel_lock anyway */
#define mx_mmap_up_write(a)
#define mx_mmap_down_write(a)
#elif defined __powerpc__ && LINUX_VERSION_CODE == KERNEL_VERSION(2,4,4)
/* __down_write and __up_write not defined/exported */
/* not getting the semaphore is still safe for this version
   because we have the page_table_lock and the kernel_lock anyway */
#define mx_mmap_up_write(a)
#define mx_mmap_down_write(a)
#else
#define mx_mmap_up_write(a) up_write(&(a)->mmap_sem)
#define mx_mmap_down_write(a) down_write(&(a)->mmap_sem)
#endif
#define mx_mm_lock(a) do { mx_mmap_down_write(a); \
   spin_lock(&(a)->page_table_lock); } while (0)
#define mx_mm_unlock(a) do { spin_unlock(&(a)->page_table_lock); \
  mx_mmap_up_write(a); } while (0)

/*
 * Figure out what, if any, high-memory support routines to use
 */
#if LINUX_XX >= 26 || defined HAVE_PTE_OFFSET_MAP_NESTED

/* nothing to do since we use this interface in our code */

#elif defined HAVE_PTE_KUNMAP /* suse version */

#define pte_unmap pte_kunmap
#define pte_unmap_nested pte_unmap
#define pte_offset_map pte_offset
#define pte_offset_map_nested pte_offset

#elif defined HAVE_PTE_OFFSET_MAP /* redhat version */

#define pte_offset_map_nested pte_offset_map2
#define pte_unmap_nested pte_unmap2

#else /* not using high pte stuff */

#define pte_offset_map pte_offset
#define pte_offset_map_nested pte_offset
#define pte_unmap(pte)
#define pte_unmap_nested(pte)

#endif

#ifdef HAVE_REMAP_PAGE_RANGE_5ARGS
#define mx_remap_page_range(vma,start,phys,len,prot) \
	remap_page_range ((vma),(start),(phys),(len),(prot))
#else
#define mx_remap_page_range(vma,start,phys,len,prot) \
	remap_page_range ((start),(phys),(len),(prot))
#endif

#ifdef HAVE_REMAP_PFN_RANGE
 #define mx_remap_pfn_range remap_pfn_range
#else
 #define mx_remap_pfn_range(vma,start,pfn,len,prot) \
          mx_remap_page_range(vma,start,(pfn)<<PAGE_SHIFT,len,prot)
#endif

#ifdef HAVE_IO_REMAP_PFN_RANGE
  #define mx_io_remap_pfn_range io_remap_pfn_range
#else
  #define mx_io_remap_pfn_range mx_remap_pfn_range
#endif

#if LINUX_XX < 26 && !defined page_to_pfn
static inline  unsigned long page_to_pfn(struct page *page)
{
  mal_assert(VALID_PAGE(page));
  return page_to_phys(page) / PAGE_SIZE;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#define mx_TryLockPage(page) (!trylock_page(page))
#else
#define mx_TryLockPage  TestSetPageLocked
#endif

/* work queue stuff */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
#define mx_lxx_delayed_work work_struct
#define MX_LXX_INIT_WORK INIT_WORK
#define MX_LXX_INIT_DELAYED_WORK INIT_WORK
#define MX_WORK_STRUCT_ARG_CONTAINER_OF(ptr, type, member) ptr
#define mx_lxx_delayed_work work_struct
#else
#define mx_lxx_delayed_work delayed_work
#define MX_LXX_INIT_WORK(w,f,d) INIT_WORK(w,f)
#define MX_LXX_INIT_DELAYED_WORK(w,f,d) INIT_DELAYED_WORK(w,f)
#define MX_WORK_STRUCT_ARG_CONTAINER_OF(ptr, type, member) container_of(ptr, type, member)
#endif
#define mx_lxx_schedule_work schedule_work
#define mx_lxx_flush_scheduled_work flush_scheduled_work


#include <linux/interrupt.h>

#define mx_pages_to_mb(x) ((x) >> (20-PAGE_SHIFT))

#ifdef HAVE_OLD_PAGE_COUNT
#if !defined page_count && LINUX_XX <= 24
#define page_count(page) atomic_read(&(page)->count)
#endif
#define MX_PAGE_COUNT_TO_ZERO 1
#define mx_p_count count
#else
#define MX_PAGE_COUNT_TO_ZERO 0
#define mx_p_count _count
#endif

#if LINUX_XX >= 26
#define mx_pci_name(dev) pci_name(dev)
#else
#define mx_pci_name(dev) dev->slot_name
#endif

#ifndef likely
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#if LINUX_XX <= 24
#define wait_on_page_locked wait_on_page
#endif


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0) && !defined module_param
#define module_param(name, type, perm) MODULE_PARM(name, _MX_PARM_##type)
#define _MX_PARM_ulong "l"
#define _MX_PARM_charp "s"
#define _MX_PARM_int "i"
#else
#include <linux/moduleparam.h>
#endif

#ifdef HAVE_CLASS_SIMPLE
typedef struct class_simple mx_class_t;
#define mx_class_create class_simple_create
#define mx_class_destroy class_simple_destroy
#define mx_class_device_create class_simple_device_add
#define mx_class_device_destroy(class, dev) class_simple_device_remove(dev)
#else
typedef unsigned mx_class_t;
#define mx_class_create(o,n) 0
#define mx_class_destroy(c)
#define mx_class_device_create(c, dev, op, name)
#define mx_class_device_destroy(class, dev)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10) && (defined CONFIG_ZLIB_INFLATE || defined CONFIG_ZLIB_INFLATE_MODULE)
/* we only use zlib when compiling with kbuild (>= 2.6.10) */
#define MX_ARCH_ZLIB_INFLATE 1
#include <linux/zlib.h>

#define mx_zlib_inflate zlib_inflate

static inline int
mx_zlib_inflateInit(z_stream *zs)
{
  int rv;
  zs->workspace = kmalloc(zlib_inflate_workspacesize(), GFP_KERNEL);
  if (zs->workspace == NULL) {
    printk("mx WARN:Failed to alloc workspace for inflateInit\n");
    return Z_MEM_ERROR;
  }
  rv = zlib_inflateInit(zs);
  if (rv != Z_OK)
    kfree(zs->workspace);
  return rv;
}

static inline int
mx_zlib_inflateEnd(z_stream *zs)
{
  int rv;
  rv = zlib_inflateEnd(zs);
  kfree(zs->workspace);
  return rv;
}
#endif /* >= 2.6.10 && CONFIG_ZLIB_INFLATE */

#ifndef PCI_EXT_CAP_ID_ERR
#define PCI_EXT_CAP_ID_ERR	1
#endif

#ifndef PCI_ERR_CAP
#define PCI_ERR_CAP 24
#endif

#ifndef PCI_ERR_CAP_ECRC_GENC
#define PCI_ERR_CAP_ECRC_GENC	0x00000020
#endif

#ifndef PCI_ERR_CAP_ECRC_GENE
#define PCI_ERR_CAP_ECRC_GENE	0x00000040
#endif

#ifndef PCI_CAP_ID_EXP
#define PCI_CAP_ID_EXP 0x10
#endif

#ifndef PCI_EXP_FLAGS_TYPE
#define PCI_EXP_FLAGS_TYPE 0x00f0
#endif

#ifndef PCI_EXP_TYPE_ROOT_PORT
#define PCI_EXP_TYPE_ROOT_PORT 0x4
#endif

#ifndef PCI_EXP_DEVCTL
#define PCI_EXP_DEVCTL 0x8
#endif

#ifndef PCI_EXP_LNKCTL
#define PCI_EXP_LNKCTL 0x10
#endif

#ifndef PCI_EXP_LNKCTL_DISABLE
#define PCI_EXP_LNKCTL_DISABLE 0x10
#endif

#if !defined VM_HUGETLB && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define VM_HUGETLB 0
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#define mx_skb_padto(skb,len) (!(skb = skb_padto(skb,len)))
#else
#define mx_skb_padto skb_padto
#endif

#ifdef HAVE_SKB_LINEARIZE_2ARGS
#define mx_skb_linearize(skb) skb_linearize(skb, GFP_ATOMIC)
#else
#define mx_skb_linearize skb_linearize
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
/* uts namespace introduced in 2.6.19 */
#define mx_current_utsname system_utsname
#else
#define mx_current_utsname current->nsproxy->uts_ns->name
#endif

/* CHECKSUM_HW replaced by CHECKSUM_PARTIAL and CHECKSUM_COMPLETE in 2.6.19 */
#include <linux/skbuff.h>
#ifndef CHECKSUM_PARTIAL
#define CHECKSUM_PARTIAL CHECKSUM_HW
#endif
#ifndef CHECKSUM_COMPLETE
#define CHECKSUM_COMPLETE CHECKSUM_HW
#endif

#ifndef IA32_MSR_CR_PAT
#define IA32_MSR_CR_PAT 0x277
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
static inline void
mx_on_each_cpu(void (*func)(void *), void *arg, int wait)
{
  (void)smp_call_function(func, arg, 0, wait);
  func(arg);
}
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,26)
#define mx_on_each_cpu(func,arg,wait) on_each_cpu(func,arg,0,wait)
#else
#define mx_on_each_cpu on_each_cpu
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
#define mx_get_board_numa_node(is) pcibus_to_node(is->arch.pci_dev->bus)
#else
#define mx_get_board_numa_node(is) (-1)
#endif

#ifndef HAVE_SKB_TRANSPORT_OFFSET
#define skb_transport_offset(skb) ((skb)->h.raw - (skb)->data)
#define skb_transport_header(skb) ((skb)->h.raw)
#endif

/* SA_SHIRQ has been removed in 2.6.22 (and set to something breaking the build explicitly)
 * so we always use IRQF_SHARED and define it to the old SA_SHIRQ if needed */
#ifndef IRQF_SHARED
#define IRQF_SHARED SA_SHIRQ
#endif

/* pci_module_init has been removed in 2.6.22.
 * pci_register_driver is the right one to use, but its return value changed in the past.
 * It now returns 0 or -ERROR while it used to return <count> or -ERROR before 2.6.10.
 * We workaround this by changing positive return value into 0.
 */
static inline int mx_pci_module_init(struct pci_driver *drv) {
  int err = pci_register_driver (drv);
  return err > 0 ? 0 : err;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
/* pci_get_bus_and_slot() appeared in 2.6.19 to replace pci_find_slot() deprecated in 2.6.23 */
#define mx_pci_get_bus_and_slot pci_get_bus_and_slot
#define mx_pci_get_dev_put pci_dev_put
#define mx_pci_get_device pci_get_device
#else
#define mx_pci_get_bus_and_slot pci_find_slot
#define mx_pci_get_dev_put(d) /* nothing */
#define mx_pci_get_device pci_find_device
#endif

#if LINUX_XX <= 24 && !defined pte_offset_kernel
#define pte_offset_kernel pte_offset
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,23)
#define netif_tx_disable netif_stop_queue
#endif

#ifndef SET_NETDEV_DEV
#define SET_NETDEV_DEV(a,b)
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,19)
#define csum_offset csum
#endif

#if defined(CONFIG_INET_LRO_MODULE) || defined(CONFIG_INET_LRO)
#define HAVE_LRO
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#define HAVE_SSET_COUNT
#endif

#define MX_ETHTOOL_OPS_TYPE typeof(*((struct net_device *)NULL)->ethtool_ops)

#define MX_VLAN_ETH_FRAME_LEN 1518

#ifndef __GFP_COMP
#define MX_GFP_COMP 0
#else
#define MX_GFP_COMP __GFP_COMP
#endif

#ifndef __GFP_NOWARN
#define MX_GFP_NOWARN 0
#else
#define MX_GFP_NOWARN __GFP_NOWARN
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,0)
#define pci_set_consistent_dma_mask(a,b) (0)
#define mx_dma_alloc_coherent(pdev, size, dmap, flags) pci_alloc_consistent(pdev, size, dmap)
#define mx_dma_free_coherent(pdev, size, addr, dma) pci_free_consistent(pdev, size, addr, dma) 
#else
#define mx_dma_alloc_coherent(pdev, size, dmap, flags) dma_alloc_coherent(&(pdev)->dev, size, dmap, flags)
#define mx_dma_free_coherent(pdev, size, addr, dma) dma_free_coherent(&(pdev)->dev, size, addr, dma) 
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,25)
#define find_task_by_vpid find_task_by_pid
#endif

#undef min_low_pfn
#define min_low_pfn 0

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
#define mx_mmiowb()
#else
#define mx_mmiowb() mmiowb()
#endif

#ifdef HAVE_CURRENT_EUID
#define mx_current_euid() current_euid()
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
#define mx_current_euid() current->cred->euid
#else
#define mx_current_euid() current->euid
#endif
#endif

/* mc_list changes in 2.6.35 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
#define myri_mc_list_type netdev_hw_addr
#define myri_mc_addr addr
#else
#define myri_mc_list_type dev_mc_list
#define myri_mc_addr dmi_addr
#ifndef netdev_for_each_mc_addr
#define netdev_for_each_mc_addr(ha, dev)        \
        for (ha = dev->mc_list; ha != NULL; ha = ha->next)
#endif
#endif 


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
#define myri_node_to_cpumask(node) cpumask_of_node(node)
#define myri_cpumask_and(dstp,src1p,src2p) cpumask_and(dstp,src1p,src2p)
#else
#define myri_cpumask_and(dstp,src1p,src2p) cpus_and(*(dstp),*(src1p),*(src2p))
#define myri_node_to_cpumask(node) &(node_to_cpumask(node))
#endif

#ifdef HAVE_KTHREAD_RUN
#include <linux/kthread.h>
#endif


#endif /* _mx_linux_compat_h_ */
