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

/* modifications for MX kernel lib made by
 * Brice.Goglin@ens-lyon.org (LIP/INRIA/ENS-Lyon) */

static const char __idstring[] = "@(#)$Id$";

#define mx_printf printk
#include "mx_arch.h"
#if MX_KERNEL_LIB
#include "mx_arch_klib.h"
#endif
#include "mx_misc.h"
#include "mx_instance.h"
#include "mx_malloc.h"
#include "mx_dma_map.h"
#include "mx_ether_common.h"
#include "mal_stbar.h"
#include "myri_version.h"
#include <linux/random.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/kmod.h>
#include <linux/poll.h>
#include <linux/console.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>


#if MX_KERNEL_LIB
int myri_init_klib(void);
void myri_finalize_klib(void);
#endif

#if (MAL_CPU_powerpc64 || MAL_CPU_powerpc)
#include <asm/pci-bridge.h>
#endif
#if LINUX_XX >= 26
#include <linux/cdev.h>
static mx_class_t *mx_class;
#endif

#if MAL_CPU_powerpc64

#if LINUX_XX >= 26
#include <asm/iommu.h>
typedef struct iommu_table *mx_ppc64_iommu_t;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)

#define MX_PPC64_IOMMU(pdev) ((pdev)->dev.archdata.dma_data)
#define MX_PPC64_HAS_IOMMU(pdev) MX_PPC64_IOMMU(pdev)
#define MX_IOMMU_PAGE_SIZE IOMMU_PAGE_SIZE
#else

#ifndef PCI_DN
#define PCI_DN(dn) (dn)
#endif
#ifndef PCI_GET_DN
#define PCI_GET_DN(dev) ((struct device_node *)((dev)->sysdata))
#endif
#define MX_PPC64_IOMMU(dev) (PCI_DN(PCI_GET_DN(dev))->iommu_table)
#define MX_IOMMU_PAGE_SIZE PAGE_SIZE
#define MX_PPC64_HAS_IOMMU(pdev) (PCI_GET_DN(pdev) && MX_PPC64_IOMMU(pdev))
#endif /* 2.6.20 */

#ifdef HAVE_IT_MAPSIZE
#define MX_PPC64_IOMMU_SIZE(iommu) ((iommu)->it_mapsize)
#else
#define MX_PPC64_IOMMU_SIZE(iommu) ((iommu)->it_size / (PAGE_SIZE / MX_IOMMU_PAGE_SIZE) )
#endif /* HAVE_IT_MAPSIZE */

#else /* LINUX_XX >= 26 */
typedef struct TceTable *mx_ppc64_iommu_t;
#define MX_PPC64_IOMMU(dev) (((struct device_node *)((dev)->sysdata))->tce_table)
#define MX_PPC64_IOMMU_SIZE(tbl) (tbl->size*(PAGE_SIZE/sizeof(union Tce)))
#include <asm/prom.h>
#include <asm/pci_dma.h>
#endif 	/* LINUX_XX >= 26 */
#endif	/* MAL_CPU_powerpc64 */

#if MX_OPTERON_IOMMU
#include <asm/k8.h>
#include <asm/gart.h>
#endif

#define MX_MAJOR 220

MODULE_AUTHOR("Maintainer: help@myri.com");
MODULE_DESCRIPTION("Myricom " MYRI_DRIVER_STR " driver");
MODULE_LICENSE("Dual BSD/GPL");
#if LINUX_XX >= 26
MODULE_VERSION(MYRI_VERSION_STR);
#endif

static unsigned myri_udev = 0;

static char *myri_mx_mapper_path;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
unsigned myri_ether_rx_frags = 1;
#else
unsigned myri_ether_rx_frags = 0;
#endif
unsigned myri_ether_rx_alloc_order = 0;
unsigned myri_ether_lro = 1;
unsigned myri_ether_csum = 1;
unsigned myri_ether_ifname_legacy = 1;
unsigned myri_msi = MX_MSI ? -1 : 0; /* auto=-1  on=1 off=0 */
unsigned myri_ecrc = 1;
int myri_bus[8];
unsigned myri_bus_nb = 0;
unsigned myri_bh_intr = 0;
unsigned myri_bar64_loc = 32;
unsigned myri_std_uc = 0;
int myri_unaligned_force = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,14)
unsigned myri_ether_vlan_csum_fixup = 1;
#else
unsigned myri_ether_vlan_csum_fixup = 0;
#endif
static unsigned mx_page_pat_attr = MX_LINUX_PAGE_NOCACHE;
static unsigned myri_console = 0;
static unsigned myri_iommu_max_mb = 0;

module_param(myri_debug_mask, int, S_IRUGO | S_IWUSR);
module_param(myri_intr_coal_delay, int, S_IRUGO);
module_param(myri_max_instance, int, S_IRUGO);
module_param(myri_security_disabled, int, S_IRUGO);
module_param(myri_max_endpoints, int, S_IRUGO);
module_param(myri_recvq_vpage_cnt, int, S_IRUGO);
module_param(myri_eventq_vpage_cnt, int, S_IRUGO);
module_param(myri_iommu_max_mb, int, S_IRUGO);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
module_param_array(myri_bus, int, &myri_bus_nb, S_IRUGO);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
module_param_array(myri_bus, int, myri_bus_nb, S_IRUGO);
#endif
module_param(myri_udev, int, S_IRUGO);
module_param(myri_ether_rx_frags, int, S_IRUGO);
module_param(myri_ether_rx_alloc_order, int, S_IRUGO);
module_param(myri_ether_lro, int, S_IRUGO);
module_param(myri_msi, int, S_IRUGO);

module_param(myri_ecrc, int, S_IRUGO);
module_param(myri_bh_intr, int, S_IRUGO);
module_param(myri_z_loopback, int, S_IRUGO);
module_param(myri_ether_csum, int, S_IRUGO);
module_param(myri_ether_pause, int, S_IRUGO);
module_param(myri_ether_ifname_legacy, int, S_IRUGO);
module_param(myri_parity_recovery, int, S_IRUGO | S_IWUSR);
module_param(myri_recover_from_all_errors, int, S_IRUGO | S_IWUSR);
module_param(myri_mx_max_host_queries, int, S_IRUGO | S_IWUSR);
module_param(myri_pcie_down, int, S_IRUGO);
module_param(myri_bar64_loc, int, S_IRUGO);
module_param(myri_unaligned_force, int, S_IRUGO);
module_param(myri_std_uc, int, S_IRUGO);
module_param(myri_pcie_down_on_error, int, S_IRUGO | S_IWUSR);
module_param(myri_ethernet_bitmap, int, S_IRUGO | S_IWUSR);
module_param(myri_ether_vlan_csum_fixup, int, S_IRUGO | S_IWUSR);
#if MX_DMA_DBG
module_param(myri_dma_pfn_max, int, S_IRUGO);
#endif
module_param(myri_force, int, S_IRUGO);
module_param(myri_jtag, int, S_IRUGO);
module_param(myri_mx_max_macs, int, S_IRUGO);
module_param(myri_console, int, S_IRUGO);
module_param(myri_irq_sync, int, S_IRUGO);
#if defined(CONFIG_MTRR)
static int myri_pat_idx = 1;
static int mx_pat_failed = 0;
module_param(myri_pat_idx, int, S_IRUGO);
#endif
module_param(myri_clksync_period, int, S_IRUGO | S_IWUSR);
module_param(myri_clksync_error, int, S_IRUGO | S_IWUSR);
module_param(myri_clksync_error_count, int, S_IRUGO);
module_param(myri_clksync_verbose, int, S_IRUGO | S_IWUSR);
module_param(myri_snf_rings, int, S_IRUGO | S_IWUSR);
module_param(myri_snf_flags, int, S_IRUGO | S_IWUSR);

module_param(myri_msi_level, int, S_IRUGO | S_IWUSR);

#ifdef CONFIG_PCI_MSI
static int myri_nvidia_msi = 1;
module_param(myri_nvidia_msi, int, S_IRUGO);
#endif
module_param(myri_override_e_to_f, int, S_IRUGO);

module_param(myri_mx_max_nodes, int, S_IRUGO);
module_param(myri_mx_max_send_handles, int, S_IRUGO);
module_param(myri_mx_max_rdma_windows, int, S_IRUGO);
module_param(myri_mx_small_message_threshold, int, S_IRUGO);
module_param(myri_mx_medium_message_threshold, int, S_IRUGO);
module_param(myri_mx_mapper_path, charp, S_IRUGO);

module_param(myri_mac, charp, S_IRUGO);

static DEFINE_SPINLOCK(mx_pin_lock);
DEFINE_SPINLOCK(mx_print_lock);
/*
static spinlock_t mx_pin_lock = SPIN_LOCK_UNLOCKED;
spinlock_t mx_print_lock = SPIN_LOCK_UNLOCKED;
*/

static struct completion mx_watchdog_completion;
wait_queue_head_t mx_watchdog_queue;
static int mx_module_is_exiting;

#if MX_DMA_DBG
static uint8_t **mx_dma_dbg_counts;
#endif

#ifndef HAVE_UNLOCKED_IOCTL
#define unlocked_ioctl ioctl
#define mx_ioctl_return_t int
#define MX_INODE_ARG struct inode *inode,
#else
#define mx_ioctl_return_t long
#define MX_INODE_ARG
#endif

static unsigned long mx_pci_dev_base(struct pci_dev *dev, int bar);
ssize_t mx_read (struct file*, char*, size_t, loff_t*);
ssize_t mx_write (struct file*, const char*, size_t, loff_t*);
mx_ioctl_return_t mx_ioctl (MX_INODE_ARG struct file*, unsigned int,
			    unsigned long);
int mx_open (struct inode*, struct file*);
int mx_release (struct inode*, struct file*);
int mx_mmap (struct file*, struct vm_area_struct*);
unsigned int mx_poll(struct file *filp, poll_table *wait);

static void mx_cleanup_linear_map(struct pci_dev *pdev);
static void myri_snf_intr_tx_flush_work(void *arg);
#if MYRI_SNF_KAGENT
static void myri_snf_kagent_work(void *arg);
#endif


struct file_operations mx_fops = {
  read: mx_read,
  write: mx_write,
  unlocked_ioctl: mx_ioctl,
#ifdef HAVE_COMPAT_IOCTL
  compat_ioctl: mx_ioctl,
#endif
  open: mx_open,
  release: mx_release,
  mmap: mx_mmap,
  poll: mx_poll,
  owner: THIS_MODULE
};


static void mx_console_write(struct console *c, const char *buf, unsigned count);

static struct {
  mx_instance_state_t *is;
  struct console linux_cons;
} mx_cons = {
  NULL, 
  {
    .name = "myri",
    .write = mx_console_write,
    .flags = CON_PRINTBUFFER,
  }
};


#ifdef CONFIG_PCI_MSI

int mx_hyper_msi_cap_on(struct pci_dev *pdev, int force)
{
  uint8_t cap_off;
  int nbcap = 0;
  
  cap_off = PCI_CAPABILITY_LIST - 1;
  /* go through all caps looking for a hypertransport msi mapping */
  while (pci_read_config_byte(pdev, cap_off + 1, &cap_off) == 0 &&
	 nbcap++ <= 256/4) {
    uint32_t cap_hdr;
    if (cap_off == 0 || cap_off == 0xff)
      break;
    cap_off &= 0xfc;
    /* cf hypertransport spec, msi mapping section */
    if (pci_read_config_dword(pdev, cap_off, &cap_hdr) == 0
	&& (cap_hdr & 0xff) == 8 /* hypertransport cap */
	&& (cap_hdr & 0xf8000000) == 0xa8000000 /* msi mapping */) {
      if (cap_hdr & 0x10000) /* msi mapping cap enabled */
	return 1;
      if (force) {
	cap_hdr |= 0x10000;
	pci_write_config_dword(pdev, cap_off, cap_hdr);
	return 1;
      }
    }
  }
  return 0;
}

static int
mx_use_msi(struct pci_dev *pdev)
{
  if (myri_msi == 1 || myri_msi == 0)
    return myri_msi;
  
#if MAL_CPU_x86_64 || MAL_CPU_x86 || MAL_CPU_ia64
  /*  find root complex for our device */
  while (pdev->bus && pdev->bus->self) {
    pdev = pdev->bus->self;
    /* avoid potential infinite loop on non-x86 */
    if (pdev == pdev->bus->self)
      return 1;
  }
  /* go for it if chipset is intel, or has hypertransport msi cap */
  if (pdev->vendor == PCI_VENDOR_ID_INTEL)
    return 1;

  if (pdev->vendor == 0x10de
      && (pdev->device == 0x005d
	  || (pdev->device >= 0x0374 /* nforce-mcp55 */
	      && pdev->device <= 0x0378 /* nforce-mcp55 */))) {
    /*  check or force nvidia  hypertransport msi cap */
    pdev = mx_pci_get_bus_and_slot(pdev->bus->number, 0);
    if (pdev) {
      if (mx_hyper_msi_cap_on(pdev, myri_nvidia_msi)) {
	mx_pci_get_dev_put(pdev);
	return 1;
      }
      mx_pci_get_dev_put(pdev);
    }
  } else 
    return mx_hyper_msi_cap_on(pdev, 0);
  /* default off */
  return 0;
#else /* x86/x64/ia64 */
  return 1;
#endif
}
#endif /* CONFIG_PCI_MSI */

static uint32_t *
mx_mmio_ext_config(struct pci_dev *dev)
{
  static unsigned long base = 0;

  uint32_t *ptr32 = 0;
  uint32_t pci_id;
  unsigned long dev_off;

  dev_off = ((unsigned long)dev->bus->number * 0x00100000UL
	     + (unsigned long)dev->devfn * 0x00001000UL);

  if (base)
    goto cached;

  /* we find the base address where ext-conf-space is available
     for a few popular nvidia chipsets */
  if (dev->vendor == 0x10de) {
    if (dev->device == 0x005d) {
      struct pci_dev *ck804;
      ck804 = mx_pci_get_bus_and_slot(0, 0);
      if (ck804) {
	if (ck804->vendor == 0x10de && ck804->device == 0x005e) {
	  u16 word;
	  pci_read_config_word(ck804, 0x90, &word);
	  base = ((unsigned long)word & 0xfff) << 28;
	}
	mx_pci_get_dev_put(ck804);
      }      
    } else if (dev->device >= 0x0374 && dev->device <= 0x0378) {
      struct pci_dev *mcp55;
      mcp55 = mx_pci_get_bus_and_slot(0, 0);
      if (mcp55) {
	if (mcp55->vendor == 0x10de && mcp55->device == 0x369) {
	  u16 word;
	  pci_read_config_word(mcp55, 0x90, &word);
	  base = ((unsigned long)word & 0x7ffeU) << 25;
	}
	mx_pci_get_dev_put(mcp55);
      }      
    }
  }
  if (!base)
    return NULL;

 cached:

  ptr32 = (uint32_t *) ioremap(base + dev_off, 4096);
  if (!ptr32)
    return NULL;

  pci_id = *ptr32;
  if (pci_id != dev->vendor + (dev->device << 16)) {
    MX_WARN(("%s: Ext-conf-space at unknown address, contact help@myri.com\n",
	     mx_pci_name(dev)));
    base = 0;
    iounmap(ptr32);
    return NULL;
  }
  return ptr32;
}

static int
mx_read_ext_config_dword(struct pci_dev *dev, int where, u32 *val)
{
	uint32_t *ptr32;
	int status = pci_read_config_dword(dev, where, val);
	if (status && (ptr32 = mx_mmio_ext_config(dev))) {
		*val = ptr32[where / 4];
		iounmap(ptr32);
		return 0;
	}
	return status;
}

static int
mx_write_ext_config_dword(struct pci_dev *dev, int where, u32 val)
{
	uint32_t *ptr32;
	int status = pci_write_config_dword(dev, where, val);
	if (status &&  (ptr32 = mx_mmio_ext_config(dev))) {
		ptr32[where / 4] = val;
		iounmap(ptr32);
		return 0;
	}
	return status;
}

static int
mx_find_ext_capability(struct pci_dev *dev, unsigned cap_id)
{
  unsigned cap = 0x100;
  int nbcap = 0;

  while (cap >= 0x100 && nbcap++ < 512) {
    uint32_t dw;
    if (mx_read_ext_config_dword(dev, cap, &dw) != 0)
      break;
     if ((dw & 0xffff) == cap_id)
      return cap;
    cap = dw >> 20;
  }
  return 0;
}

static void
mx_pcie_bridge_conf(mx_instance_state_t *is, struct pci_dev *bridge)
{
  unsigned cap;
  unsigned err_cap;
  int ret;
  uint16_t cmd, cap_flags;
  uint32_t dw;

  pci_read_config_word(bridge, PCI_COMMAND, &cmd);
  pci_write_config_word(bridge, PCI_COMMAND, cmd & ~PCI_COMMAND_SERR);
  cap = pci_find_capability(bridge, PCI_CAP_ID_EXP);
  if (cap) {
    pci_read_config_word(bridge, cap + PCI_EXP_DEVCTL, &cmd);
    pci_write_config_word(bridge, cap + PCI_EXP_DEVCTL, cmd & ~0xf /* ~(ce|fe|nfe|ur)*/);
  }
  if (bridge->vendor == 0x8086
      && ((bridge->device >= 0x3595 && bridge->device <= 0x359a) /* e7520 */
	  )) {
    uint16_t lnk_sta;
    if (pci_read_config_word(bridge, cap + MX_PCI_EXP_LNK_STA, &lnk_sta) == 0
	&& MX_PCI_EXP_LNK_WIDTH(lnk_sta) == 8) {
      is->arch.is_unaligned = 1;
      if (bridge->device >= 0x3595
	  && bridge->device <= 0x359a
	  && pci_read_config_dword(bridge, 0x144, &dw) == 0) {
	/* e7x20 have a pcie-surprise-down like bit in the "unit error regs", mask at 0x144 bit 19 */
	pci_write_config_dword(bridge, 0x144, dw | 0x800);
      }
    }
  }


  if (!myri_ecrc)
    return;


  /* check if bridge is root port */
  cap = pci_find_capability(bridge, PCI_CAP_ID_EXP);
  if (!cap)
    return;
  pci_read_config_word(bridge, cap + PCI_CAP_FLAGS, &cap_flags);
  if (((cap_flags & PCI_EXP_FLAGS_TYPE) >> 4) != PCI_EXP_TYPE_ROOT_PORT)
    return;

  
  cap = mx_find_ext_capability(bridge, PCI_EXT_CAP_ID_ERR);
  /* nvidia ext cap is not always linked in ext cap chain */
  if (!cap
      && bridge->vendor == 0x10de /* nvidia */
      && ((bridge->device == 0x005d /* ck804_pcie */
	   || (bridge->device >= 0x0374 /* nforce-mcp55 */
	       && bridge->device <= 0x0378 /* nforce-mcp55 */))))
    cap = 0x160;
  
  if (!cap)
    return;
  
  ret = mx_read_ext_config_dword(bridge, cap + PCI_ERR_CAP, &err_cap);
  if (ret) {
    MX_INFO(("failed reading ext-conf-space of %s\n", mx_pci_name(bridge)));
    MX_INFO(("\t pci=nommconf in use? or buggy/incomplete/absent acpi MCFG attr?\n"));
    return;
  }
  if (!(err_cap & PCI_ERR_CAP_ECRC_GENC))
    return;
  
  err_cap |= PCI_ERR_CAP_ECRC_GENE;
  MX_INFO(("Enabling ECRC on upstream bridge %s\n",
	   mx_pci_name(bridge)));
  mx_write_ext_config_dword(bridge, cap + PCI_ERR_CAP, err_cap);
} 


static void
mx_bar64_reloc(struct pci_dev * dev)
{
#if MAL_CPU_x86_64
  struct pci_dev *bridge = dev->bus->self;
  uint32_t dw;
  uint16_t w;
  uint64_t base, limit;

  if (dev->device != 8 || !bridge
      || bridge->vendor != PCI_VENDOR_ID_INTEL
      || bridge->device < 0x3595
      || bridge->device > 0x359a)
    return;
  
  if (pci_resource_start(dev,0) >= (1ULL << 32))
    return;
  
  if (myri_bar64_loc < (MX_LINUX_PFN_MAX >> 20)) {
    MX_INFO(("Cannot shift Bar0 to %dGB, max_mem=%ldGB\n", 
	     myri_bar64_loc, MX_LINUX_PFN_MAX >> 20));
    return;
  }
#if 0
  if (list_entry(&dev->bus->devices, struct pci_dev, bus_list) != dev ||
      dev->bus_list->next != &dev->bus_devices) {
    MX_INFO(("Cannot shift Bar0, not alone on bus\n"));
  }
#endif
  pci_read_config_word(bridge, PCI_PREF_MEMORY_BASE, &w);
  base = (w & ~0xfU) << 16;
  pci_read_config_word(bridge, PCI_PREF_MEMORY_LIMIT, &w);
  limit = ((w & ~0xfU) << 16) + 0x100000U;
  pci_read_config_dword(bridge, PCI_PREF_BASE_UPPER32, &dw);
  base += (uint64_t)dw << 32;
  pci_read_config_dword(bridge, PCI_PREF_LIMIT_UPPER32, &dw);
  limit += (uint64_t)dw << 32;
  if (base != pci_resource_start(dev,0) 
      || limit != pci_resource_start(dev,0) + pci_resource_len(dev,0)) {
    printk("%s: prefetch  space [0x%llx,0x%llx[ from %s is not exactly our bar\n", 
	   pci_name(dev), base, limit, pci_name(bridge));
    return;
  }
  if (myri_bar64_loc < 4 || myri_bar64_loc & 3) {
    MX_INFO(("Invalid value for myri_bar64_loc:0x%x\n", myri_bar64_loc));
    return;
  }
  pci_write_config_dword(dev, PCI_BASE_ADDRESS_1, myri_bar64_loc / 4);
  pci_write_config_dword(bridge, PCI_PREF_BASE_UPPER32, myri_bar64_loc / 4);
  pci_write_config_dword(bridge, PCI_PREF_LIMIT_UPPER32, myri_bar64_loc / 4);
  pci_resource_start(dev,0) += myri_bar64_loc * (1ULL << 30);
  pci_resource_end(dev,0) += myri_bar64_loc * (1ULL << 30);
  MX_INFO(("Moved bar0 to %d GB\n", myri_bar64_loc));
  return;
#endif
}


static void
mx_vm_close(struct vm_area_struct *area)
{
  mx_endpt_state_t *es = area->vm_private_data;
  mx_mutex_enter(&es->sync);
  if (es->arch.mm == area->vm_mm)
    es->arch.mm = NULL;
  mx_mutex_exit(&es->sync);
}

static struct vm_operations_struct mx_vm_ops = {
  .close = mx_vm_close,
};

#if MX_DMA_DBG
#if MAL_CPU_powerpc64
#define MX_NUM_BUS_PAGES (1U << 20)
#else
#define MX_NUM_BUS_PAGES num_physpages
#endif

#undef check_bitmap_word
/* to debug the debugging code :-), sanity check between
   mx_dma_dbg_count/bitmap, 
   this function is not called by default, see mx_dma_map.h */
void
check_bitmap_word(mx_instance_state_t *is, unsigned long addr, int line)
{
  int i;
  unsigned pfn, val;
  pfn = addr / 4096;
  pfn -= pfn % 32;
  val = ntohl(is->lanai.dma_dbg_bitmap[pfn / 32]);
  for (i=0;i<32;i++) {
    if (!!(val & (1 << i)) != !!mx_dma_dbg_counts[pfn / 4096][pfn % 4096 + i]) {
      MX_INFO(("check_bitmap_word:addr=0x%x000,word=0x%x,page_count=%d, line=%d\n",
	       pfn + i, val, mx_dma_dbg_counts[pfn / 4096][pfn % 4096 + i], line));
      break;
    }
  }
}

uint8_t *mx_get_lock_count(unsigned long pfn)
{
  uint8_t *table;
  mal_assert(pfn < MX_NUM_BUS_PAGES);
  table = mx_dma_dbg_counts[pfn / 4096];
  mal_always_assert(table);
  return table + (pfn % 4096);
}
#endif


void
mx_spin(uint32_t usecs)
{
  if (usecs == 0 && !in_interrupt ()) {
      /* if usecs == 0, just do the sched_yield() equivalent */
    schedule();
  } else if (usecs < 100 || in_interrupt ()) {
    udelay (usecs);
  } else {
    /* do not want to call udelay for long time. */
    /* let be uninterruptible to be sure the delay is respected */
    set_current_state(TASK_UNINTERRUPTIBLE);
    schedule_timeout (((usecs * HZ) / 1000000) + 1);
  }
}

/*
 * find the physical address of either a kernel page or a user pager
 * by walking the page tables (see Rubini p.287)
 *
 * NOTE: cannot be called from interrupt handler for user-space
 * addresses since we use the current MM context
 */

static inline int
mx_page_is_valid(struct page *page, pte_t *pte)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
  return VALID_PAGE(page);
#else
  unsigned long pfn = pte_pfn(*pte);
  return pfn_valid(pfn);
#endif

}

struct page *
mx_kva_to_page (mx_instance_state_t * is,
		unsigned long addr, int kernel,
		struct mm_struct *mm)
{
  pgd_t *pgd;
#ifdef PUD_SHIFT
  pud_t *pud;
#else
  pgd_t *pud;
#endif
  pmd_t *pmd;
  pte_t *pte;
  struct page *page;
  void *ptr;
  int valid;

  MAL_DEBUG_PRINT(MAL_DEBUG_KVA_TO_PHYS, 
		  ("%s: (0x%lx, %d)\n", __FUNCTION__, addr, kernel));

  if (kernel)
    {
      MAL_DEBUG_PRINT(MAL_DEBUG_KVA_TO_PHYS, ("kernel\n"));
      /* if kernel:
         if vaddr in low range, conversion is done by translation (most cases).
         if vaddr after high_memory (vmalloc), we deal with the segment offset
         via VMALLOC_VMADDR */

      ptr = (void *) addr;
      if ((addr >= PAGE_OFFSET) && (addr < (unsigned long) high_memory))
	{
	  MAL_DEBUG_PRINT(MAL_DEBUG_KVA_TO_PHYS, ("low\n"));
	  return virt_to_page (ptr);
	}

      /* beware, some variables are not exported on ppc 2.2.x */
      if (addr >= VMALLOC_START && addr < VMALLOC_END)
	return vmalloc_to_page((void*)addr);
      
      MX_WARN(("kva_to_page: cannot translate 0x%lx\n", addr));
      return NULL;
    }

  pgd = pgd_offset(mm ? mm : current->mm, addr);

  if (!pgd)
    {
      return (0);
    }

  /* first level */
  if (pgd_none (*pgd))
    {
      return (0);
    }
  if (pgd_bad (*pgd))
    {
      MX_WARN(("mx_kvirt_to_page: bad PGD\n"));
      return (0);
    }

  /* second level */
#ifdef PUD_SHIFT
  pud = pud_offset (pgd, addr);

  if (pud_none (*pud))
    {
      return (0);
    }
  if (!pud_present (*pud))
    {
      return (0);
    }
  if (pud_bad (*pud))
    {
      MX_WARN(("mx_kvirt_to_page: bad PUD\n"));
      return (0);
    }
#else
  pud = pgd;
#endif

  pmd = pmd_offset (pud, addr);

  if (pmd_none (*pmd))
    {
      return (0);
    }
  if (!pmd_present (*pmd))
    {
      return (0);
    }
  if (pmd_bad (*pmd))
    {
      MX_WARN(("mx_kvirt_to_page: bad PMD\n"));
      return (0);
    }

  /* last level */
  pte = pte_offset_map (pmd, addr);
  if (pte_none (*pte))
    {
      goto pte_error;
    }
  if (!pte_present (*pte))
    {
      goto pte_error;
    }
  if (!pte_write (*pte))
    {
      goto pte_error;
    }

  page = pte_page(*pte);
  valid = mx_page_is_valid(page, pte);
  if (!valid)
    printk("page is not valid\n");
  pte_unmap (pte);
  return valid ? page : 0;

 pte_error:
  pte_unmap(pte);
  return 0;
}

ssize_t
mx_read(struct file* filp, char* buff, size_t count, loff_t* offp)
{
  int len = 0;
  int status = 0;
  int minr;
  int unit;
  char *c = 0;
  struct inode *inode = filp->f_dentry->d_inode;

  minr = minor(inode->i_rdev);

  unit = minor(inode->i_rdev) / 2;

  status = mx_instance_status_string(unit, &c, &len);
  if (status)
    return -status;

  if (*offp > len)
    goto abort_with_c;

  if (*offp + count > len)
    count = len - *offp;

  status = copy_to_user(buff, c + *offp, count);
  if (status) {
    status = -EFAULT;
    goto abort_with_c;
  }
  *offp += count;
  status = count;

 abort_with_c:
  mx_kfree(c);
  return status;  
}

ssize_t
mx_write(struct file* filp, const char* buff, size_t count,
	 loff_t* offp)
{
  return 0;
}

unsigned int
mx_poll(struct file *filp, poll_table *wait)
{
  int minr;
  struct inode *inode = filp->f_dentry->d_inode;
  mx_endpt_state_t *es;
  mx_instance_state_t *is;
  unsigned int mask = 0;

  minr = minor(inode->i_rdev);
  
  es = filp->private_data;

  is = es->is;

  /* check to see if the mcp died, since the raw endpoint opener
     will want to know about it */
  if (mx_is_dead(is))
    return POLLERR;

  if (es->es_type == MYRI_ES_MX || es->es_type == MYRI_ES_SNF_RX)
  {
    int revents;
    poll_wait(filp, &es->arch.poll_waitq, wait);

    revents = myri_poll_events_pending(es);
    if (!revents) {
      myri_poll_prep(es);
      return 0;
    }

    /* epoll always needs us to interrupt */
    if (wait == NULL) myri_poll_prep(es);

    if (revents & MYRI_WAIT_RX_EVENT)
      mask |= POLLIN;
    if (revents & MYRI_WAIT_TX_EVENT)
      mask |= POLLOUT;
    return mask;
  }


  return POLLERR;
}

void
myri_update_numa_config(mx_endpt_state_t *es)
{
  cpumask_t cpumap;
  int cpu = -1;
  int node_id = -1;

  if (current)
    cpumap = current->cpus_allowed;
  else
    cpumap = CPU_MASK_NONE;

  if ((cpu = first_cpu(cpumap)) != NR_CPUS) {
    node_id = cpu_to_node(cpu);
    for_each_cpu_mask(cpu, cpumap) {
      if (node_id != cpu_to_node(cpu)) {
        node_id = -1;
        break;
      }
    }
    if (cpus_weight(cpumap) == 1)
      cpu = first_cpu(cpumap);
    else
      cpu = -1;
  }
  es->cpu_id = cpu;
  es->node_id = node_id;
}

static int
mx_set_endpoint (struct file* filp,  unsigned int cmd, uaddr_t arg)
{
  mx_endpt_state_t *es;
  mx_set_endpt_t set_endpt;
  int unit;
  int status;
  size_t len;
  enum myri_endpt_type es_type;

  switch (cmd) {
    case MX_SET_ENDPOINT:
      es_type = MYRI_ES_MX;
      break;
    case MX_SET_RAW:
      es_type = MYRI_ES_RAW;
      break;
    case MYRI_SNF_SET_ENDPOINT_RX:
    case MYRI_SNF_SET_ENDPOINT_RX_RING:
    case MYRI_SNF_SET_ENDPOINT_RX_BH:
      es_type = MYRI_ES_SNF_RX;
      break;
    case MYRI_SNF_SET_ENDPOINT_TX:
      es_type = MYRI_ES_MX;
      break;
    default:
    return EINVAL;
    break;
  }

  /* unit bounds checking was done in open, and will be 
     done again in mx_common_open */
  unit = minor(filp->f_dentry->d_inode->i_rdev) / 2;

  if (cmd == MX_SET_ENDPOINT) {
    status = mx_arch_copyin(arg, &set_endpt, sizeof(set_endpt));
    if (status)
      return EFAULT;
    if (set_endpt.endpoint < 0 || set_endpt.endpoint >= myri_max_endpoints)
      return ERANGE;
  }
  else if (cmd == MYRI_SNF_SET_ENDPOINT_TX) {
    myri_snf_tx_params_t txp;
    status = mx_arch_copyin(arg, &txp, sizeof(txp));
    if (status)
      return EFAULT;
    set_endpt.endpoint = txp.epid;
    if (set_endpt.endpoint < 0 || set_endpt.endpoint >= myri_max_endpoints)
      return ERANGE;
  }
  /* The next few endpoints just consume driver-level endpoints, not
   * MCP endpoints. */
  else if (cmd == MYRI_SNF_SET_ENDPOINT_RX)
    set_endpt.endpoint = SNF_ENDPOINT_RX;
  else if (cmd == MYRI_SNF_SET_ENDPOINT_RX_RING)
    set_endpt.endpoint = SNF_ENDPOINT_RX_RING;
  else if (cmd == MYRI_SNF_SET_ENDPOINT_RX_BH)
    set_endpt.endpoint = SNF_ENDPOINT_RX_BH;

  es = mx_kmalloc(sizeof(*es), MX_MZERO|MX_WAITOK);
  if (es == 0)
    return ENOMEM;

  es->privileged = minor(filp->f_dentry->d_inode->i_rdev) & 1;
  es->is_kernel = 0;
  es->opener.pid = mx_kgetpid();
  es->euid = mx_current_euid();
  if (sizeof(current->comm) > sizeof(es->opener.comm))
    len = sizeof(es->opener.comm);
  else
    len = sizeof(current->comm);
  bcopy(current->comm, es->opener.comm, len);

  status = mx_common_open(unit, set_endpt.endpoint, es, es_type);
  
  if (status != 0) {
    mx_kfree(es);
    return (status);
  }
  set_endpt.session_id = es->session_id;

  myri_update_numa_config(es);

  if (cmd == MX_SET_ENDPOINT)
    status = mx_arch_copyout(&set_endpt, arg, sizeof(set_endpt));
  else if (cmd == MYRI_SNF_SET_ENDPOINT_TX ||
           cmd == MYRI_SNF_SET_ENDPOINT_RX ||
           cmd == MYRI_SNF_SET_ENDPOINT_RX_RING ||
           cmd == MYRI_SNF_SET_ENDPOINT_RX_BH) {
    MX_LXX_INIT_DELAYED_WORK(&es->arch.snf.tx_flush_work,
                             (void *) myri_snf_intr_tx_flush_work, es);
    status = myri_snf_ioctl(es, cmd, arg);
    if (status != 0) {
      mx_common_close(es);
      mx_kfree(es);
      return (status);
    }
  }

  filp->private_data = es;

//  init_MUTEX(&es->arch.shm_lock);
  sema_init(&es->arch.shm_lock,1);
  init_waitqueue_head(&es->arch.shm_waitq);
  MAL_DEBUG_PRINT(MAL_DEBUG_OPENCLOSE, 
		  ("Board %d, endpoint %d opened\n", 
		   unit, set_endpt.endpoint));

  return status;
}

mx_ioctl_return_t
mx_ioctl (MX_INODE_ARG struct file* filp, unsigned int cmd,
            unsigned long arg)
{
  mx_endpt_state_t *es;
  int retval;
  int privileged;
 
  mx_dma_dbg_lock_kernel();
  privileged = minor(filp->f_dentry->d_inode->i_rdev) & 1;
  if (filp->private_data == 0) {
    switch (cmd) {
    case MX_SET_ENDPOINT:    
    case MX_SET_RAW:
    case MYRI_SNF_SET_ENDPOINT_TX:
    case MYRI_SNF_SET_ENDPOINT_RX:
    case MYRI_SNF_SET_ENDPOINT_RX_RING:
    case MYRI_SNF_SET_ENDPOINT_RX_BH:
      retval = mx_set_endpoint(filp, cmd, (uaddr_t)arg);
      break;
    default:
      retval = mx_endptless_ioctl(cmd, (uaddr_t)arg, privileged, 0);
    }
    goto done;
  }
  es = filp->private_data;

  mx_mutex_enter(&es->sync);
  es->ref_count++;
  mx_mutex_exit(&es->sync);  

  if (es->es_type == MYRI_ES_SNF_RX)
    retval = myri_snf_ioctl(es, cmd, (uaddr_t)arg);
  else
  retval = mx_common_ioctl(es, cmd, (uaddr_t)arg);
  if (retval == ENOTTY) {
    retval = mx_endptless_ioctl(cmd, (uaddr_t)arg, privileged, 0);
  }

  mx_mutex_enter(&es->sync);
  es->ref_count--;
  mx_mutex_exit(&es->sync);  

 done:
  mx_dma_dbg_unlock_kernel();

  return(-1 * retval);

}

int
mx_open (struct inode* inode, struct file* filp)
{
  int minr;
  int unit;

  mx_dma_dbg_lock_kernel();
  minr = minor(inode->i_rdev);
  unit = minor(inode->i_rdev) / 2;
  if (unit >= myri_max_instance) {
    mx_dma_dbg_unlock_kernel();
    return -ENODEV;
  }
  /* unit in bounds, open will be finished in ioctl */
  filp->private_data = 0;
  mx_dma_dbg_unlock_kernel();
  return 0;
}

int
mx_release (struct inode* inode, struct file* filp)
{
  mx_endpt_state_t *es;

  mx_dma_dbg_lock_kernel();
  es = filp->private_data;
  filp->private_data = 0;

  if (!es) { /* endpoint was never fully opened, just return 0 */
    mx_dma_dbg_unlock_kernel();
    return 0;
  }

  mx_common_close(es);
  if (es->es_type == MYRI_ES_MX) {
    cancel_delayed_work(&es->arch.snf.tx_flush_work);
    flush_scheduled_work();
  }
  mx_kfree(es);
  mx_dma_dbg_unlock_kernel();
  return 0;
}

void
mx_reserve_page(void *kva)
{
#if LINUX_VERSION_CODE  <= KERNEL_VERSION(2,6,26)
  struct page * page = mx_kva_to_page(kva, (unsigned long)kva, 1, NULL);
  set_bit(PG_reserved,&page->flags);
#endif
}

void
mx_unreserve_page(void *kva)
{
#if LINUX_VERSION_CODE  <= KERNEL_VERSION(2,6,26)
  struct page * page = mx_kva_to_page(kva, (unsigned long)kva, 1, NULL);
  clear_bit(PG_reserved,&page->flags);
#endif
}

int
mx_mmap (struct file* filp, struct vm_area_struct* vma)
{
  mx_endpt_state_t *es;
  mx_instance_state_t *is;
  unsigned long start, end, pos, off, len;
  uint64_t phys;
  pgprot_t prot;
  struct page * page;
  void *kva;
  mx_page_pin_t *dontcare;
  int status;
  int mem_type;

  es = (mx_endpt_state_t *)filp->private_data;
  if (es == NULL)
    return -1;
  is = es->is;

  mx_mutex_enter(&es->sync);
#if 0
  if (!es->arch.mm)
    es->arch.mm = vma->vm_mm;
#endif

  vma->vm_private_data = es;
  vma->vm_ops = &mx_vm_ops;
  vma->vm_flags |= VM_IO;

  start = vma->vm_start;
  end = vma->vm_end;
  off = vma->vm_pgoff << PAGE_SHIFT;
  len = end - start;
  prot = vma->vm_page_prot;

  /*
   * determine the mem type for this request
   */ 
  kva = NULL;
  status = mx_mmap_off_to_kva(es, off, &kva, &mem_type, &dontcare);
  if (status != 0) {
    MAL_DEBUG_PRINT 
      (MAL_DEBUG_KVA_TO_PHYS,
       ("status = %d, len = 0x%lx\n", status, len));
    goto abort_with_mutex;
  }

  if (mem_type == MX_MEM_HOSTMEM || mem_type == MX_MEM_HOSTMEM_UNMAPPED) {
    /*
     * loop to map all kernel pages (non-contiguous)
     */
    for (pos = 0; pos < len; pos += PAGE_SIZE) {
      /*
       * determine the kva for this request
       */
      kva = (void*)pos;
      status = mx_mmap_off_to_kva(es, off + pos, &kva, &mem_type, &dontcare);
      if (status != 0) {
	MAL_DEBUG_PRINT 
	  (MAL_DEBUG_KVA_TO_PHYS,
	   ("status = %d, pos = 0x%lx, len = 0x%lx\n", status, pos, len));
	goto abort_with_mutex;
      }
      /* 
       * remap_page_range needs a physical address
       */
      if (mem_type == MX_MEM_HOSTMEM_UNMAPPED)
	page = (struct page *)kva;
      else
	page = mx_kva_to_page(es->is, (unsigned long)kva, 1, NULL);
      if (kva && !page) {
        MX_WARN(("page is NULL and kva is %p\n", kva));
        MX_WARN(("es, off is %lu/%#lx\n", off, off));
      }
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28)
      phys = page_to_pfn(page) << PAGE_SHIFT;
      if (mx_remap_pfn_range(vma, start + pos, mx_linux_pfn(phys), PAGE_SIZE, prot)) {
	MX_WARN(("mx_remap_pfn_range failed: 0x%lx, 0x%08x%08x, 0x%lx\n",
		 (start + pos), MX_HIGHPART_TO_U32(phys),
		 MX_LOWPART_TO_U32(phys),
		 (unsigned long)PAGE_SIZE));
	goto abort_with_mutex;
      }
#else
      if (vm_insert_page(vma, start + pos, page)) {
	MX_WARN(("vm_insert_page failed: 0x%lx,pfn=0x%lx\n",
		 (start + pos), page_to_pfn(page)));
	goto abort_with_mutex;
      }
#endif
    }
  } else {
    int spec_bar = 0;
    unsigned long phys_base;

    if (MAL_IS_ZE_BOARD(is->board_type)
	&& mem_type == MX_MEM_SPECIAL)
      spec_bar = 2;
    phys_base = MX_LINUX_IOMEM2PHYS(mx_pci_dev_base(is->arch.pci_dev, spec_bar));
    /*
     * map device memory (contiguous)
     */
    switch (mem_type) {
    case MX_MEM_SRAM:
      phys = phys_base + (uintptr_t)kva - (uintptr_t)is->lanai.sram;
      break;
    case MX_MEM_CONTROL:
      phys = phys_base + (uintptr_t)kva - (uintptr_t)is->lanai.control_regs;
      break;
    case MX_MEM_SPECIAL:
      phys = phys_base + (uintptr_t)kva - (uintptr_t)is->lanai.special_regs;
      break;
    default:
      phys = 0; /* Placate compiler. */
      MX_WARN(("mx_mmap_off_to_kva returned with unknown memory type %d\n",
	       mem_type));
      mal_always_assert(0);
      break;
    }

    /* We must disable caching on device memory */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    if (myri_std_uc) 
      {
	prot = pgprot_noncached(vma->vm_page_prot);
      } 
    else
#endif
      {
      pgprot_val (prot) &= ~MX_LINUX_PAGE_CACHE;
      pgprot_val (prot) |= mx_page_pat_attr;
      }

    if (mx_io_remap_pfn_range(vma, start,  mx_linux_pfn(phys), len, prot)) {
      MX_WARN(("mx_io_remap_pfn_range failed: 0x%lx, 0x%08x%08x, 0x%lx\n",
	       start, MX_HIGHPART_TO_U32(phys),
	       MX_LOWPART_TO_U32(phys), len));
      goto abort_with_mutex;
    }
  }
  
  mx_mutex_exit(&es->sync);
  return(0);

  abort_with_mutex:

  mx_mutex_exit(&es->sync);
  return -1;
  
}

void 
mal_assertion_failed (const char *assertion, int line, const char *file)
{
  printk(MYRI_DRIVER_STR " assertion: <<%s>>  failed at line %d, file %s\n",
	 assertion, line, file);
}
/*********************************************************************
 * kernel memory allocation functions
 *********************************************************************/
/*
 * poor man's memory leak detection
 */
#if MAL_DEBUG
static int kmalloc_cnt = 0, kfree_cnt = 0;
static int vmalloc_cnt = 0, vfree_cnt = 0;
//static int ioremap_cnt = 0, iounmap_cnt = 0;
//static int dma_alloc_cnt = 0, dma_free_cnt = 0;
static int kernel_alloc_cnt = 0, kernel_free_cnt = 0;
//static int user_pin_cnt = 0, user_unpin_cnt = 0;
#endif

static unsigned long mx_max_user_pinned_pages_start;
static unsigned long mem_total_pages;
static int mx_linux_pci_driver_registered = 0;

/* with little mem, reserve half memory */
/* Found that 3/4 was too much on some boxes with linux-2.4 */
#define MX_MAX_USER_PINNED_SMALLMEM(x) (((x)*4)/8)
/* with average amount of memory, preserve a fix amount */
#define MX_MAX_SAVE_FROM_PINNED (64*1024*1024/PAGE_SIZE)
/* with a lot of mem, divide first to avoid overflow,
   and reserve a part proportional to memsize */
#define MX_MAX_USER_PINNED_BIGMEM(x) (((x)/8)*7)

/****************************************************************
 * Synchronization functions
 ****************************************************************/

void
mx_spin_lock_init(mx_spinlock_t *s, mx_instance_state_t *is, int unique, char *str)
{
  // *s = SPIN_LOCK_UNLOCKED;
  spin_lock_init(s);
}

void
mx_sync_init (mx_sync_t *s, mx_instance_state_t *is, int unique, char *str)
{
  //init_MUTEX(&s->mutex);
  sema_init(&s->mutex,1); 
  //init_MUTEX(&s->wake_sem);
  sema_init(&s->wake_sem,1); 
  atomic_set(&s->wake_cnt, 0);
  init_waitqueue_head(&s->sleep_queue);
}

void
mx_sync_reset (mx_sync_t *s)
{
  atomic_set(&s->wake_cnt, 0);
}

void
mx_sync_destroy(mx_sync_t *s)
{
}

void
mx_mutex_enter(mx_sync_t *s)
{
  down(&(s->mutex));
}

int
mx_mutex_try_enter(mx_sync_t *s)
{
  return down_trylock(&(s->mutex));
}


void
mx_mutex_exit(mx_sync_t *s)
{
  up(&(s->mutex));
}

/*****************************************************************
 * Sleep functions
 *****************************************************************/

/* The interrupt handler atomically increments WAKE_CNT each time a
   wake interrupt is received and the user threads decrementing 
   WAKE_CNT each time they claim a wake interrupt.   */

/****************
 * waking
 ****************/

/* Wake the thread sleeping on the synchronization variable. */

void
mx_wake(mx_sync_t * s)
{
  MAL_DEBUG_PRINT(MAL_DEBUG_SLEEP, ("mx_wake called on s = %p\n", s));

  /* record the wake interrupt by incrementing the wake count.  This
     need to be atomic because disabling interrupt globally on SMP 
     is very costly. */

  atomic_inc(&s->wake_cnt);
  wake_up(&s->sleep_queue);
}

/*
 * This wake_once implementation means "at least once" but it doesn't
 * guarantee "exactly once".  This is fine for all existing mx_wake_once() use
 * cases since the call sites are already serialized.
 */
int
mx_wake_once(mx_sync_t *s)
{
  if (atomic_read(&s->wake_cnt) <= 0) {
    atomic_inc(&s->wake_cnt);
    wake_up(&s->sleep_queue);
    return 1;
  }
  return 0;
}

/****************
 * sleeping
 ****************
 
 The following code claims a wake interrupt by atomically testing for a
 positive WAKE_CNT and decrementing WAKE_CNT.  We can assume we are the
 only one trying to consume wake_cnt, the caller is responsible to get a
 mutex to ensure this, so wake_cnt can only increase while we are here.
 A basic Linux rule: if you need to disable interrupts globally, your
 code is not written the right way :-) */

/* sleep until awakend, timeout or signal */

int
mx_sleep(mx_sync_t *s, int ms, int flags)
{
  long timeout;
  int ret = 0;
  DECLARE_WAITQUEUE(wait, current);

  MAL_DEBUG_PRINT(MAL_DEBUG_SLEEP, ("mx_sleep  sync = %p  ms=%d\n",
				   s, ms));
  /* take the mutex */
  down(&s->wake_sem);

  /* set the timeout.  If ms is zero, sleep forever */
  if (ms == MX_MAX_WAIT) {
    timeout = MAX_SCHEDULE_TIMEOUT;
  } else {
    timeout = ((long)ms * HZ)/1000;
    if (timeout == 0 && ms)
      timeout = 1;
  }
 
  /* put the process in the queue before testing the event */
  add_wait_queue(&s->sleep_queue, &wait);
  while (timeout > 0 && 
	 atomic_read(&s->wake_cnt) <= 0) {
    /* use MX_SLEEP_INTR to allow signals */
    if (flags & MX_SLEEP_INTR) {
      set_current_state(TASK_INTERRUPTIBLE);
      if (signal_pending(current)) {
	set_current_state(TASK_RUNNING);
	break;
      }
    } else {
      set_current_state (TASK_UNINTERRUPTIBLE);
    }
    /* test again wake_cnt after setting current->state to avoid
       race condition */
    if (atomic_read(&s->wake_cnt) <= 0)
      timeout = schedule_timeout(timeout);
    /* reset state to RUNNING in case the if was not taken */
    set_current_state(TASK_RUNNING);
  }

  remove_wait_queue(&s->sleep_queue, &wait);

  if (atomic_read(&s->wake_cnt) <= 0) {
    /* no interrupt, timed out */
    if (timeout <= 0) {
      ret = EAGAIN;
    } else {
      ret = EINTR;
      mal_always_assert(signal_pending(current));
    }
  } else {
    /* claims the interrupt */
    atomic_dec(&s->wake_cnt);
  }

  /* release the mutex */
  up(&s->wake_sem);
  MAL_DEBUG_PRINT(MAL_DEBUG_SLEEP, ("mx_sleep  sync = %p  timeout=%ld, ret = %d\n",
				   s, timeout, ret));

  return ret;
}

/***************************************************************
 * User/kernel memory page pining/unpining
 ***************************************************************/

/***************************************************************
 * User memory page locking/unlocking
 ***************************************************************/

#if MX_LINUX_NOLOCKPIN

int
mx_lock_priv_page(struct page *page, mx_page_pin_t *pin, struct vm_area_struct *vma)
{
  int already_pinned;

  if (!page) {
    /* page might be swapped or read-only, caller should force write
       swapin and try again */
    return EAGAIN;
  }
  already_pinned = page_count(page) - 1 - !!PageSwapCache(page);

  if (unlikely(page_mapcount(page) != 1) || unlikely(already_pinned < 0)) {
    MX_WARN(("BUG: found page with mapcount==%d, pagecount==%d, SwapCache=%d\n",
	     page_mapcount(page), page_count(page), PageSwapCache(page)));
    return ENXIO;
  }

  pin->page = page;
  pin->private = 1 + !already_pinned;
  if (!already_pinned)
    mx_atomic_subtract(1, &myri_max_user_pinned_pages);
  get_page(page);
  return 0;
}

void
mx_unlock_priv_page(mx_page_pin_t *pin)
{
  if (pin->private == 2)
    mx_atomic_add(1, &myri_max_user_pinned_pages);
  put_page(pin->page);
}

#endif

int
mx_lock_pages(mx_instance_state_t *is, mx_page_pin_t *pins, int npages, 
	      int flags, struct mm_struct *mm)
{
  struct vm_area_struct *vma;
  int status = 0;
  int i, j;
  unsigned locked_num = 0;
  unsigned dma_num = 0;
  int locked_zone = 0;
  struct page *page;
  mx_page_pin_t * pin;
  int priv_page = 0;
  struct page * current_huge_page = NULL;
  uint64_t current_huge_va = 0;

  if (unlikely(flags & MX_PIN_PHYSICAL)) {
    for(i=0, pin=pins; i<npages; i++, pin++) {
      /* physical address, so just find the DMA address since we
	 assume the user has already locked it. */
      mal_assert((pin->va & (PAGE_SIZE - 1)) == 0);
      page = mx_linux_phys_to_page(pin->va);
      status = mx_dma_map(is, page, &pin->dma);
      if (status != 0) {
	for(j=0; j<i; j++)
	  mx_dma_unmap(is, &pins[j].dma);
	break;
      }
    }
    return status;
  }

  if (unlikely(flags & MX_PIN_KERNEL)) {
    for (i=0, pin=pins; i < npages; i++, pin++) {
      /* kernel address, so just find the DMA address since in linux,
	 kernel addresses are always wired */
      page = mx_kva_to_page(is, pin->va, 1, NULL);
      mal_assert(page);
      status = mx_dma_map(is, page, &pin->dma);
      if (status != 0) {
	for(j=0; j<i; j++)
	  mx_dma_unmap(is, &pins[j].dma);
	break;
      }
    }
    return status;
  }

  if (!mm)
    mm = current->mm;
  mx_mmap_down_write(mm);
  vma = find_vma(mm, pins[0].va);
  /* find_vma does not necessarily returns a VMA that includes the addr, 
   * which might lie outside any VMA */
  if (!vma || vma->vm_end <= pins[0].va) {
    MX_WARN(("No vma for addr: 0x%lx\n", (unsigned long)pins[0].va));
    status = EIO;
    goto out_error;
  }
  priv_page = !(vma->vm_flags & (VM_SHARED | VM_HUGETLB));
  if (priv_page) {
    spin_lock(&mm->page_table_lock);
    spin_lock(&mx_pin_lock);
    locked_zone = 1;
  }
  if (atomic_read(&myri_max_user_pinned_pages) < npages) {
    status = ENOMEM;
    MX_WARN(("max_user_pinned_pages limit(%ld mb) reached, available=%ld kb\n", 
	     mx_pages_to_mb(mx_max_user_pinned_pages_start),
	     atomic_read(&myri_max_user_pinned_pages)*PAGE_SIZE/1024));
    goto out_error;
  }
  for (dma_num=0, pin=pins; dma_num<npages; dma_num++, pin++) {
    int retry = 0;

    do {
#if (defined CONFIG_HUGETLB_PAGE) && (defined HPAGE_SHIFT) && !(defined MAL_CPU_ia64)
      /* HPAGE_SHIFT is a non-exported variable on ppc between at least 2.6.15 and 2.6.18.
       * HPAGE_SHIFT is defined to hpage_shift on ia64, and hpage_shift is not exported (at least since 2.6.9)
       */
      if (current_huge_page) {
	if ((pin->va >> HPAGE_SHIFT) == (current_huge_va >> HPAGE_SHIFT)) {
	  /* we are in the same huge page ? */
	  page = pin->page = current_huge_page + ((pin->va - current_huge_va) >> PAGE_SHIFT);
	  pin->huge_page_already_pinned = 1;
	  break; /* next step is mx_dma_map */
	} else {
	  /* we went out of the huge page */
	  current_huge_page = NULL;
	  current_huge_va = 0;
	}
      }
#endif
      pin->huge_page_already_pinned = 0;
      vma = find_vma(mm, pin->va);
      if (!vma || vma->vm_end <= pin->va || priv_page != !(vma->vm_flags & (VM_SHARED | VM_HUGETLB))) {
	MX_WARN(("Bad vma (%p, fl=0x%lx,priv=%d) for addr: 0x%lx\n", 
		 vma, vma ? vma->vm_flags : 0, priv_page, (unsigned long)pin->va));
	status = EIO;
	goto out_error;
      }
      if (priv_page) {
	page = mx_kva_to_page(is, pin->va, 0, mm);
	status = mx_lock_priv_page(page, pin, vma);
      } else {
	if (get_user_pages(current, current->mm, (unsigned long)pin->va, 1, 1,
			   0, &page, NULL) == 1) {
	  pin->private = priv_page;
	  pin->page = page;
	  if (vma->vm_flags & VM_HUGETLB) {
	    current_huge_page = page;
	    current_huge_va = pin->va;
	  }
	  status = 0;
	} else {
	  status = EFAULT;
	}
      }
      if (status == EAGAIN) {
	/* tlb miss or locked page */
	if ((vma->vm_flags & (VM_MAYWRITE | VM_WRITE | VM_SHARED)) == VM_MAYWRITE) {
	  MX_PRINT_ONCE(("Successfully forcing private ro mapping"
			 " to rw for registration\n"));
	  vma->vm_flags |= VM_WRITE;
	}
	
	spin_unlock (&mx_pin_lock);
	spin_unlock (&mm->page_table_lock);
	locked_zone = 0;
	if (get_user_pages(current, mm, pin->va, 1, 1, 0, &page, 0) != 1) {
	  status = EFAULT;
	  MX_WARN(("No valid page found by get_user_page for this address: 0x%lx\n",
		   (unsigned long) pin->va));
	  goto out_error;
	}
	/* MX only lock private pages, mapped once in the AS,
	   the page cannot be locked by ourselves */
	wait_on_page_locked(page);
	page_cache_release(page);
	page = 0;
	if (retry++ >= 3) {
	  MX_WARN(("tlb miss of address %lx 3 times in a row\n",
		   (unsigned long) pin->va));
	  status = ENXIO;
	  goto out_error;
	}
	spin_lock(&mx_pin_lock);
	spin_lock(&mm->page_table_lock);
	locked_zone = 1;
      } else if (status) {
	goto out_error;
      }
    } while (status);

    mal_always_assert(MX_LINUX_NOLOCKPIN || !priv_page || PageLocked(page));
    locked_num += 1;
    status = mx_dma_map(is, page, &pin->dma);
    if (status) {
      static int seen;
      if (!seen++) {
	MX_WARN(("mx_dma_map failed\n"));
	goto out_error;
      }
    }
  }
  if (priv_page) {
    spin_unlock (&mx_pin_lock);
    spin_unlock(&mm->page_table_lock);
  }
  mx_mmap_up_write(mm);
  
  return status;

 out_error:
  if (!locked_zone) {
    spin_lock(&current->mm->page_table_lock);
    spin_lock(&mx_pin_lock);
  }
  for (i=0; i<dma_num; i++) {
    mx_dma_unmap(is, &pins[i].dma);
  }
  for (i=0; i<locked_num; i++) {
    if (priv_page)
      mx_unlock_priv_page(pins + i);
    else
      put_page(pins[i].page);
  }
  spin_unlock(&mx_pin_lock);
  spin_unlock(&mm->page_table_lock);
  mx_mmap_up_write(mm);
  return status;
}

int 
mx_pin_host_pages(mx_instance_state_t *is, mx_page_pin_t *pins,
		  mcp_dma_addr_t *mdesc, int npages, int flags,
		  uint64_t memory_context)
{
  int i, status;
  status = mx_lock_pages(is, pins, npages, flags, (struct mm_struct *)(unsigned long) memory_context);
  if (status)
    return status;
  
  for (i=0;i<npages;i++) {
    mdesc[i].high = htonl(pins[i].dma.high);
    MAL_WRITEBAR();
    mdesc[i].low = htonl(pins[i].dma.low);
  }
  return 0;
}


void
mx_unpin_host_pages(mx_instance_state_t *is, mx_page_pin_t *pins, int npages, int flags)
{
  mx_page_pin_t *pin;
  int i;

  spin_lock(&mx_pin_lock);
  for (i=0, pin=pins; i<npages; i++, pin++) {

    mx_dma_unmap(is, &pin->dma);
    
    if (pin->huge_page_already_pinned)
      continue;

    if (flags & (MX_PIN_KERNEL|MX_PIN_PHYSICAL)) {
      continue;
    }
    
    mal_always_assert(pin->page);
    if (pin->private) {
      mx_unlock_priv_page(pin);
    } else {
      put_page(pin->page);
    }
  }
  spin_unlock(&mx_pin_lock);
}



int 
mx_pin_page(mx_instance_state_t *is, mx_page_pin_t *pin, int flags, uint64_t memory_context)
{
  return mx_lock_pages(is, pin, 1, flags, (struct mm_struct *)(unsigned long) memory_context);
}

void
mx_unpin_page(mx_instance_state_t *is, mx_page_pin_t *pin, int flags)
{
  mx_unpin_host_pages(is, pin, 1, flags);
}


int
mx_rand(void)
{
  int ret;
  
  get_random_bytes(&ret, sizeof(ret));
  return ret;
}

static int  mx_null_alloc;

void *
__mx_kmalloc (size_t len, uint32_t flags, int numa_node)
{
  void *retval;

  MAL_DEBUG_ACTION(MAL_DEBUG_MALLOC, kernel_alloc_cnt++);


  /* 64 is a safe value, anyway it will work even if the threshold
     does not exactly correspond to kmalloc internals */
  if (len == 0) {
    return &mx_null_alloc;
  } else if (len <= PAGE_SIZE) {
    retval =  kmalloc(len, GFP_KERNEL);
    if (retval)
      MAL_DEBUG_ACTION(MAL_DEBUG_MALLOC, kmalloc_cnt++);
  } else {
    retval = vmalloc_node(len, numa_node);

    if (retval) {
      MAL_DEBUG_ACTION(MAL_DEBUG_MALLOC, vmalloc_cnt++);
    }
  }
  if (retval && (flags & MX_MZERO)) {
    memset((char *)retval, 0, len);
  }
  return retval;
}

void *
mx_kmalloc (size_t len, uint32_t flags)
{
  return __mx_kmalloc(len, flags, -1);
}

void *
mx_kmalloc_node(size_t len, uint32_t flags, int numa_node)
{
  return __mx_kmalloc(len, flags, numa_node);
}

void
mx_kfree (void *ptr)
{

  MAL_DEBUG_ACTION(MAL_DEBUG_MALLOC, kernel_free_cnt++);
#if MAL_DEBUG
  if (!ptr) {
    MX_WARN(("mx_kfree: NULL pointer!!\n"));
    return;
  }
#endif
  if (ptr == &mx_null_alloc) {
  } else if ((ptr > (void *) PAGE_OFFSET) && (ptr < (void *) high_memory)) {
    MAL_DEBUG_ACTION(MAL_DEBUG_MALLOC, kfree_cnt++);
    kfree(ptr);
  } else {
    MAL_DEBUG_ACTION(MAL_DEBUG_MALLOC, vfree_cnt++);
    vfree(ptr);
  }
}

/*********************************************************************
 * memory mapping (into kernel space)
 *********************************************************************/

void *
mx_map_pci_space (mx_instance_state_t * is, int bar, uint32_t offset, uint32_t len)

{
  void *kaddr = NULL;
  unsigned long iomem = mx_pci_dev_base(is->arch.pci_dev, bar);

  MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT,
            ("mx_map_io_space(%p, 0x%x, %d)\n", is, offset, len));

  if (offset + len > pci_resource_len(is->arch.pci_dev, bar)
      || !(pci_resource_flags(is->arch.pci_dev, bar) & IORESOURCE_MEM)
      || !iomem)
    return 0;
  if (bar == 0 && (MAL_IS_ZE_BOARD(is->board_type) || offset < 0x800000))
    {
#if defined(CONFIG_MTRR) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
      kaddr = __ioremap(iomem + (unsigned long) offset, len, mx_page_pat_attr);
#elif defined(ARCH_HAS_IOREMAP_WC)
      kaddr = ioremap_wc(iomem + (unsigned long) offset, len);
#else
      kaddr = ioremap_nocache(iomem + (unsigned long) offset, len);
#endif
    }
  else
    {
      kaddr = ioremap_nocache(iomem + (unsigned long) offset, len);
    }

#if defined IO_TOKEN_TO_ADDR
  if (kaddr)
    kaddr = (void *) IO_TOKEN_TO_ADDR(kaddr);
#endif

  MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT,
            ("ioremapped 0x%p (offset 0x%x, len 0x%x)\n",
             kaddr, offset, len));

  return kaddr;
}


void
mx_unmap_io_space (mx_instance_state_t * is,
                        uint32_t len, void *kaddr)
{

  MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT,
            ("iounmapping %p (len 0x%x)\n",
             kaddr, len));

  mx_cleanup_linear_map(is->arch.pci_dev);
  iounmap(kaddr);
}

#define MX_DEFAULT_PAT 0x7040600070406ULL
#define MX_ENABLED_PAT ((MX_DEFAULT_PAT & ~(0xffULL << (myri_pat_idx * 8))) \
			      | (0x01ULL << (myri_pat_idx * 8)))

#ifdef CONFIG_X86_64
#ifndef MAXMEM
#include <asm/e820.h>
#endif /*MAXMEM*/
#endif /*CONFIG_X86_64*/

/* 
 * This function is used to work around a quirk of the linux kernel which
 * would otherwise cause our driver to leak 16MB of ram per interface
 * when PAT write-combining is used.
 *
 * Early in the boot process, Linux maps linearly all physical space
 * at: [PAGE_OFFSET, PAGE_OFFSET + <end-of-usable-physical-space]
 * ioremap() gives a new mapping for the same physical space in a
 * different virtual region (the "vmalloc" region). Linux tries to
 * keep the same attributes for the two virtual mapping of the same
 * physical space through the clumsy change_page_attr(). After
 * ioremap() has more or less established the new mapping,
 * change_page_attr() is called on the corresponding interval of the
 * "linear mapping" to fix it, and this part is confused by the
 * presence of the PAT (_PAGE_PSE) bit in the pte entries and leaks
 * memory.  This function is designed to be called prior to iounmap()
 * to clear the _PAGE_PSE bits in the linear mapping, and eliminate
 * this leak of vmalloc space.
*/

static void
mx_cleanup_linear_map(struct pci_dev *pdev)
{
#if defined(CONFIG_MTRR) && LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,25)

	pgd_t *pgd;
#ifdef PUD_SHIFT
	pud_t *pud;
#else
	pgd_t *pud;
#endif
	pmd_t *pmd;
	pte_t *pte;
	unsigned long offset;
	unsigned long addr;
	int warnings = 0;

	addr = pci_resource_start(pdev, 0);
	if (addr == 0)
		return;
#ifdef CONFIG_X86_64
	if (addr >= MAXMEM)
		return;
#else
	if (addr >= virt_to_phys(high_memory))
		return;
#endif
	for (offset = 0;offset < pci_resource_len(pdev, 0);
	     offset += PAGE_SIZE) {
		addr = (unsigned long)__va(pci_resource_start(pdev, 0) + offset);
		pgd = pgd_offset_k(addr);
		if (!pgd_present(*pgd)) {
			printk("mx_pat:pgd not present\n");
			return;
		}
#ifdef PUD_SHIFT
		pud = pud_offset(pgd, addr);
		if (!pud_present(*pud)) {
			printk("m_pat:pud not present\n");
			return;
		}
#else
		pud = pgd;
#endif
		pmd = pmd_offset(pud, addr);
		if (!pmd_present(*pmd)) {
			if (!warnings++)
				printk("mx_pat:pmd not present\n");
			continue;
		}
		if (pmd_val(*pmd) & _PAGE_PSE) {
			continue;
		}
		pte = pte_offset_kernel(pmd, addr);
		if (pte_present(*pte) && (pte_val(*pte) & _PAGE_PSE)) {
#ifdef CONFIG_X86_64
			pte->pte &= ~_PAGE_PSE;
#else
			pte->pte_low &= ~_PAGE_PSE;
#endif
		}
	}
#endif
}

#ifdef CONFIG_MTRR
static void
mx_enable_pat(void *info)
{
#ifdef CONFIG_RT_MUTEXES
  static atomic_t lock = {0};
#else
  static spinlock_t lock = SPIN_LOCK_UNLOCKED;
#endif

  u64 val;
  static int warned = 0;
  mx_instance_state_t *is = (mx_instance_state_t *)info;
  
  unsigned id = smp_processor_id();
  uint32_t low, high;
  uint8_t type;
  
#ifdef CONFIG_RT_MUTEXES
  /* hand roll spinlock to avoid panic on RT linux */
  preempt_disable();
  while (atomic_cmpxchg(&lock, 0, 1) != 0)
    ;
#else  
  spin_lock(&lock);
#endif
  rdmsr(IA32_MSR_CR_PAT, low, high);
  if (id == 0)
    MX_INFO(("%s: CPU%d: PAT = 0x%x%08x\n", is->is_name, id, high, low));
  val = ((u64)high << 32ULL) | low;
  type = (uint8_t)(val >> (myri_pat_idx * 8));
  if (type != (uint8_t)(MX_DEFAULT_PAT >> (myri_pat_idx * 8))
      && type != 0x01 /* WC */) {
    if (!warned) {
      warned = 1;
      MX_WARN(("%s: CPU%d: existing PAT "
	       "has non-default value =  0x%x%08x\n", 
               is->is_name, id, high, low));
      MX_WARN(("%s: PAT not enabled!\n", is->is_name));
    }
    goto abort_with_lock;
  }
  
  if (type != 0x01) {
    val &= ~(0xffULL << (myri_pat_idx * 8));
    val |= 0x01ULL << (myri_pat_idx * 8);
    wrmsr(IA32_MSR_CR_PAT, (uint32_t)val, (uint32_t)(val >> 32));
    rdmsr(IA32_MSR_CR_PAT, low, high);
    if (id == 0)
      MX_INFO(("%s: CPU%d: new PAT = 0x%x%08x\n", is->is_name, id, high, low));
    val = ((u64)high << 32ULL) | low;
  }
  
abort_with_lock:
  type = (uint8_t)(val >> (myri_pat_idx * 8));
  if (type != 0x01)
    mx_pat_failed = 1;
  
#ifdef CONFIG_RT_MUTEXES
  atomic_set(&lock, 0);
  preempt_enable();
#else  
  spin_unlock(&lock);
#endif
}
#endif

static inline void 
mx_setup_writecomb(mx_instance_state_t *is)
{
  is->arch.mtrr = -1;
#ifdef CONFIG_MTRR
  /* MTRR is buggy in linux versions <= ~2.6.22, mostly triggered on recent 64bit procs
     PAT does not work reliably on Pentium IIIand early P4 (not dangerous though)
     reasonable compromise (not perfect) is to never use MTRR on 64-bit kernel  */
  if (sizeof(long) == 4 || myri_pat_idx == -1)
    is->arch.mtrr = mtrr_add(mx_pci_dev_base(is->arch.pci_dev, 0), is->board_span,
			     MTRR_TYPE_WRCOMB, 1);
  if (is->arch.mtrr >= 0) {
    MX_INFO(("Board %d: Write Combining enabled through mtrr %d\n", mx_num_instances, is->arch.mtrr));
    return;
  }
  if (myri_pat_idx == -1) {
    MX_INFO(("PAT disabled through myri_pat_idx\n"));
    return;
  }
  if (!test_bit(X86_FEATURE_PSE, (unsigned long*)boot_cpu_data.x86_capability)) {
    MX_INFO(("PAT disabled because pse not enabled\n"));
    return;
  }
#ifdef CONFIG_XEN		/* _PAGE_PSE is not yet supported by Xen */
  MX_INFO(("CONFIG_XEN enabled, if really using Xen, myri_pat_idx=-1 might be needed\n"));
#endif
  mx_on_each_cpu(mx_enable_pat, is, 1);
  if (!mx_pat_failed) {
    MX_INFO(("%s: Using PAT index %d\n", is->is_name, myri_pat_idx));
    /* note the double negation below is used to turn an integer into a
       boolean */
    mx_page_pat_attr =  (!!(myri_pat_idx & 4) * _PAGE_PSE +
			 !!(myri_pat_idx & 2) * _PAGE_PCD +
			 !!(myri_pat_idx & 1) * _PAGE_PWT);
  }
#endif /* CONFIG_MTRR */
}

static inline void 
mx_teardown_writecomb(mx_instance_state_t *is)
{
#ifdef CONFIG_MTRR
  if (is->arch.mtrr >= 0)
    mtrr_del(is->arch.mtrr, mx_pci_dev_base(is->arch.pci_dev, 0), is->board_span);
#endif /* CONFIG_MTRR */
  is->arch.mtrr = -1;
}

static irqreturn_t
mx_linux_intr (int irq, void *instance_id, struct pt_regs *regs)
{
  mx_instance_state_t *is = instance_id;
  int handled;
  unsigned long flags;

  if (myri_bh_intr == 0) {
    handled = mx_common_interrupt(is);
    return IRQ_RETVAL(handled);
  }

  /* spurious if no interrupt descriptor */
  if (is->intr.ring[is->intr.slot].valid == 0)
    return IRQ_RETVAL(0);
  
  spin_lock_irqsave(&is->arch.intr_pending_lock, flags);
  if (!is->arch.intr_pending) {
    if (!is->using_msi)
      is->board_ops.disable_interrupt(is);
    mx_lxx_schedule_work(&is->arch.intr_work);
  }
  is->arch.intr_pending += 1;
  spin_unlock_irqrestore(&is->arch.intr_pending_lock, flags);
  return IRQ_RETVAL(1);
}

static void
mx_devfs_register (char *devname, mx_instance_state_t *is, int minor)
{
  char name[10];
  umode_t mode;
#if LINUX_XX <= 24
  devfs_handle_t *devfs_handle;
#endif
  int priv = minor & 1;

  mode = priv ? (S_IRUSR | S_IWUSR) : (S_IRUGO | S_IWUGO);

  sprintf(name, "%s%d", devname, minor/2);
#if LINUX_XX <= 24
  devfs_handle = &is->arch.devfs_handle[priv];
#endif
  
#if LINUX_XX >= 26
  devfs_mk_cdev(MKDEV(MX_MAJOR, minor), S_IFCHR | mode, name);
  if (myri_udev) {
    mx_class_device_create(mx_class, MKDEV(MX_MAJOR, minor),
			  NULL, name);
  }
#else
  *devfs_handle =
    devfs_register(NULL, name, DEVFS_FL_DEFAULT, MX_MAJOR, minor,
                    S_IFCHR | mode, &mx_fops, 0);
#endif
}


/****************************************************************
 * mx_linux_register_ioctl32()
 * Registers 32 bit ioctls on 64 bit systems.
 ****************************************************************/
#if (MAL_CPU_x86_64 || MAL_CPU_powerpc64) && !defined HAVE_COMPAT_IOCTL
extern int register_ioctl32_conversion(unsigned int cmd,
				       int (*handler)(unsigned int,
						      unsigned int,
                                                      unsigned long,
                                                      struct file *));
extern int unregister_ioctl32_conversion(unsigned int cmd);

static int
mx_linux_register_ioctl32(void)
{
  unsigned int i;
  int err;

  MX_INFO(("Registering 32 bit ioctls\n"));
  for (i=1; i<= MYRI_LAST_IOCTL; i++)
    {
      if ((err = register_ioctl32_conversion(MYRI_IO(i), NULL)) != 0)
	{
	  MX_WARN(("Couldn't register 32 bit ioctl %d (0x%x)\n",
		   i, MYRI_IO(i)));
	  return err;
	}
    }
  return 0;
}

static int
mx_linux_unregister_ioctl32 (void)
{
  int i, err;
  for (i=1; i<= MYRI_LAST_IOCTL; i++)
    {
      if ((err = unregister_ioctl32_conversion(MYRI_IO(i))) != 0)
	return err;
    }
  return 0;

}
#endif /* MAL_CPU_x86_64 || MAL_CPU_powerpc64*/

static unsigned long 
mx_pci_dev_base(struct pci_dev *dev, int bar)
{
  unsigned long base;

  base = pci_resource_start (dev, bar);

#if MAL_CPU_powerpc64 && LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,19)
  /* - on ppc64 with eeh on, the kernel hides the real address without
     providing any way to map its fake handle to use space. In
     both cases get the information from the PCI register who never lies. */
  if ((base >> 60UL) > 0) {
    /* we got either a fake (token), or a already mapped address */
    unsigned int bus_base;
    struct pci_controller *hose = PCI_DN(PCI_GET_DN(dev))->phb;
    pci_read_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, &bus_base);
    bus_base &= PCI_BASE_ADDRESS_MEM_MASK;
    MX_WARN(("Linux faking pci_resource_start:pci_resource_start=0x%lx,"
	     "bus_base=0x%x,hose->pci_mem_offset=%lx\n",
	     pci_resource_start (dev,0), bus_base,
	     hose ? hose->pci_mem_offset : 0));
    if (bus_base && hose)
      base = bus_base + hose->pci_mem_offset;
  }	
#endif /* MAL_CPU_powerpc64 */
  return base;
}

#if LINUX_XX >= 26

static struct cdev myri_cdev = {
  .owner  = THIS_MODULE,
};


static int
mx_linux_class_init(void)
{
  if (myri_udev) {
    mx_class = mx_class_create(THIS_MODULE, "myri");
    if (mx_class == NULL) {
      MX_WARN(("mx_class_create returned %p\n", mx_class));
      return ENXIO;
    }
  }
  return 0;
}

static void
mx_linux_class_fini(void)
{
  if (myri_udev) {
    mx_class_destroy(mx_class);
  }
}

static int
mx_linux_cdev_init(void)
{
  int err;
  dev_t myri_dev = MKDEV(MX_MAJOR, 0);

  err = register_chrdev_region(myri_dev, 2*myri_max_instance, "myri");
  if (err != 0) {
    MX_WARN(("register_chrdev_region failed for mx devices with status %d\n", err));
    goto out;
  }
  kobject_set_name(&myri_cdev.kobj, "myri");
  cdev_init(&myri_cdev, &mx_fops);
  err = cdev_add(&myri_cdev, myri_dev, 2*myri_max_instance);
  if (err != 0) {
    MX_WARN(("cdev_add() failed for mx devices with status %d\n", err));
    goto out_with_mx_region;
  }
  return 0;

 out_with_mx_region:
  unregister_chrdev_region(myri_dev, 2*myri_max_instance);
 out:
  return err;
}

static void
mx_linux_cdev_fini(void)
{
  cdev_del(&myri_cdev);
  unregister_chrdev_region(MKDEV(MX_MAJOR, 0), 2*myri_max_instance);
}

#endif /* LINUX_XX >= 26 */

static void
mx_linux_pci_map_init(mx_instance_state_t *is)
{
#if MAL_CPU_powerpc64
  struct pci_dev *pdev;
  int pages_put_aside;
  int myri_users = 0, other_users = 0;
  mx_ppc64_iommu_t tbl = MX_PPC64_IOMMU(is->arch.pci_dev);
  if (!tbl) {
    MX_INFO(("%s: no iommu found\n", is->is_name));
    return;
  }
  pdev = 0;
  while ((pdev = mx_pci_get_device(PCI_ANY_ID, PCI_ANY_ID, pdev))) {
    if (pdev->hdr_type != PCI_HEADER_TYPE_NORMAL)
      continue;
    if (MX_PPC64_HAS_IOMMU(pdev) &&
	MX_PPC64_IOMMU(pdev) == tbl) {
      if (pdev == is->arch.pci_dev || pdev->vendor == MX_PCI_VENDOR_MYRICOM)
	myri_users += 1;
      else 
	other_users += 1;
    }
  }
  if (myri_users == 0) {
    MX_WARN(("Did not find ourselves in PCI list!?!?!\n"));
    myri_users = 1;
  }
  /* use 3/4 of a iommu for myri devices (or all but 64MB if not shared) */

  pages_put_aside = MX_PPC64_IOMMU_SIZE(tbl)/4;
  /* if we have a dedicated IOMMU don't put more than 
     64MB aside */
  if (!other_users && pages_put_aside > 32*1024*1024 / PAGE_SIZE)
    pages_put_aside = 32*1024*1024 / PAGE_SIZE;
  atomic_set(&is->arch.free_iommu_pages, 
	     (MX_PPC64_IOMMU_SIZE(tbl) - pages_put_aside)/ myri_users);
  is->arch.has_iommu = 1;
  MX_INFO(("%s: using %ld MB of iommu (Table is %ld Mbytes), users=%d/%d\n",
	   is->is_name, (long)mx_pages_to_mb(atomic_read(&is->arch.free_iommu_pages)),
	   (long)mx_pages_to_mb(MX_PPC64_IOMMU_SIZE(tbl)),
	   myri_users, other_users));
#if 0 /* MX_DMA_DBG */
  /* FIXME */
  is->lanai.dma_dbg_lowest_addr = tbl->it_offset * PAGE_SIZE * PAGE_SIZE;
#endif
#endif
#if MX_OPTERON_IOMMU
  if (num_k8_northbridges) {
    unsigned aper_base_32, aper_order;
    /* stolen from pci-gart_64.c, need to be rewritten */
    pci_read_config_dword(k8_northbridges[0], AMD64_GARTAPERTUREBASE, &aper_base_32);
    pci_read_config_dword(k8_northbridges[0], AMD64_GARTAPERTURECTL, &aper_order);
    aper_order = (aper_order >> 1) & 7;
    
    is->arch.aper_base = aper_base_32 & 0x7fff;
    is->arch.aper_base <<= 25;
    
    is->arch.aper_size = (32 * 1024 * 1024) << aper_order;
    if (!is->arch.aper_base 
	|| is->arch.aper_base + is->arch.aper_size > 0x100000000UL 
	|| !is->arch.aper_size) {
      is->arch.aper_base = 0;
      is->arch.aper_size = 0;
    } else {
      struct page * page;
      dma_addr_t bus;
      page = alloc_page(GFP_KERNEL);
      bus = pci_map_page(is->arch.pci_dev, page, 0, PAGE_SIZE, PCI_DMA_BIDIRECTIONAL);
      if (bus >= is->arch.aper_base && bus < is->arch.aper_base + is->arch.aper_size) {
	MX_INFO(("Detected forced IOMMU from 0x%llx->0x%llx (%d MB)\n", is->arch.aper_base, is->arch.aper_base + is->arch.aper_size, is->arch.aper_size >> 20));
	is->arch.has_iommu = 1;
	atomic_set(&is->arch.free_iommu_pages,
		   is->arch.aper_size * 3 / 4 >> PAGE_SHIFT);
	MX_INFO(("%s: setting iommu usage to %d MB\n", is->is_name, pages_to_mb(atomic_read(&is->arch.free_iommu_pages))));
      }
      pci_unmap_page(is->arch.pci_dev, bus, PAGE_SIZE, PCI_DMA_BIDIRECTIONAL);
      put_page(page);
    }
  }
#endif
  if (myri_iommu_max_mb) {
    is->arch.has_iommu = 1;
    atomic_set(&is->arch.free_iommu_pages,
	       myri_iommu_max_mb * (1 << (20 - PAGE_SHIFT)));
    MX_INFO(("%s: module parameter setting max iommu usage to %d MB\n", is->is_name, myri_iommu_max_mb));
  }
}


void myri_linux_clock_timer(unsigned long data)
{
  mx_instance_state_t * is = (void *)data;
  if (!mx_module_is_exiting) {
    myri_clock_sync_now(is);
    is->arch.clock_timer.expires =
      jiffies + (HZ * myri_clksync_period) / 1000;
    add_timer(&is->arch.clock_timer);
  }
}

void myri_linux_kwindow_timer(unsigned long data)
{
  mx_instance_state_t * is = (void *)data;
  if (!mx_module_is_exiting) {
    is->kernel_window->jiffies = jiffies;
    is->arch.kwindow_timer.expires = jiffies + 1;
    add_timer(&is->arch.kwindow_timer);
  }
}

static void
myri_snf_intr_tx_flush_work(void *arg)
{
  mx_endpt_state_t *es = MX_WORK_STRUCT_ARG_CONTAINER_OF(arg, mx_endpt_state_t, arch.snf.tx_flush_work);
  struct snf__tx_state *ss = es->snf.tx_sp.tx_s;

  if (es->arch.snf.tx_flush_cnt == ss->tx_flush_cnt) {
    /* No new flushes, try again and if unsuccessful, reschedule work queue */
    if (!myri_snf_intr_tx_try_flush(es)) {
      schedule_delayed_work(&es->arch.snf.tx_flush_work, 1);
    }
  }
}

void
myri_snf_handle_tx_intr(mx_endpt_state_t *es)
{
  struct snf__tx_state *ss = es->snf.tx_sp.tx_s;

  /* Try to flush any outstanding batched packets, If unsuccessful,
   * reschedule the flush */
  if (!myri_snf_intr_tx_try_flush(es)) {
    es->arch.snf.tx_flush_cnt = ss->tx_flush_cnt;
    schedule_delayed_work(&es->arch.snf.tx_flush_work, 0);
  }
}

static void
mx_do_bh_intr(void *arg)
{
  unsigned long flags;
  mx_instance_state_t *is = MX_WORK_STRUCT_ARG_CONTAINER_OF(arg, mx_instance_state_t, arch.intr_work);

  mx_common_interrupt(is);

  spin_lock_irqsave(&is->arch.intr_pending_lock, flags);
  mal_always_assert(is->arch.intr_pending);
  if (is->using_msi) {
    is->arch.intr_pending -= 1;
    if (is->arch.intr_pending) {
      /* race reschedule */
      mx_lxx_schedule_work(&is->arch.intr_work);
    }
  } else {
    is->board_ops.enable_interrupt(is);
    is->arch.intr_pending = 0;
  }
  spin_unlock_irqrestore(&is->arch.intr_pending_lock, flags);
}

/****************************************************************
 * mx_linux_create_instance
 *
 * Initializes the myrinet card specified.  If the card is
 *   initialized correctly, it increments mx_num_instances
 *   and adds it into the device array.
 * Arguments:
 *   dev - a pointer to the pci structure for the myrinet card
 * Returns:
 *   0 if card was initialized correctly
 *   -ENODEV otherwise
 ****************************************************************/

/* create a new device. Only at end and if no error occurs, we link it
   and increment mx_linux_num_instance. */
static int
mx_linux_create_instance (struct pci_dev *dev)
{
  mx_instance_state_t *is = NULL;
  int status = 0, dontcare;
  uint32_t class;
  unsigned short vendor, device;
  uint8_t rev;

  MAL_DEBUG_PRINT 
    (MAL_DEBUG_BOARD_INIT,
     ("Using mx_linux_create_instance for instance %d\n",
      mx_num_instances));

  if (pci_enable_device(dev)) {
    MX_WARN(("%s:  pci_enable_device failed\n",
	     mx_pci_name(dev)));
    return -ENODEV;
  }
  pci_set_master(dev);

  /*
   * lots of this could be moved to arch-independent code
   * there are just sanity checks + filling of mx_pci_config
   */
  pci_read_config_dword(dev, PCI_CLASS_REVISION, &class);
  rev = (uint8_t)class;
  MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT,
		    ("Myrinet PCI probe device = %d  revision = 0x%x\n",
		     dev->devfn, class));
  if (class == 0xffffffff) {
    MX_INFO(("PCI config class/rev for %s is 0x%x, did card disappeared?\n",
	     mx_pci_name(dev), class));
    status = -ENODEV;
    goto abort;
  }
  dontcare = pci_set_consistent_dma_mask(dev, (dma_addr_t)~0ULL);

  pci_read_config_word(dev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word(dev, PCI_DEVICE_ID, &device);

  MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT,
		  ("myri_pci_probe testing vendor=0x%x  device=0x%x\n",
		   vendor, device));

  if (vendor != MX_PCI_VENDOR_MYRICOM
      || (device != MX_PCI_DEVICE_MYRINET &&
	  device != MX_PCI_DEVICE_Z4E &&
	  device != MX_PCI_DEVICE_Z8E &&
	  device != MX_PCI_DEVICE_Z8E_9)) {
    MX_WARN(("Device Id %x:%x for %s is not recognized\n", 
	     vendor, device, mx_pci_name(dev)));
    status = -ENODEV;
    goto abort;
  }

  if ((device == MX_PCI_DEVICE_MYRINET && rev < 4) 
#if MX_2G_ENABLED == 0
      || device == MX_PCI_DEVICE_MYRINET
#endif
#if MX_10G_ENABLED == 0
      || device != MX_PCI_DEVICE_MYRINET
#endif
      )  {
    MX_WARN(("Build (%s) does not have support for Myrinet Board at %s (%02x:%02x rev %d)\n",
	     (MX_2G_ENABLED && MX_10G_ENABLED) ? "2G+10G" : 
	     MX_2G_ENABLED ? "2G" : MX_10G_ENABLED ? "10G" : "?", 
	     mx_pci_name(dev), vendor, device, rev));
    status = -ENODEV;
    goto abort;
  }

  is = (void *) mx_kmalloc(sizeof (*is), MX_MZERO);
  if (!is) {
    MX_WARN(("couldn't get memory for instance_state\n"));
    status = -ENOMEM;
    goto abort;
  }
  snprintf(is->is_name_default, sizeof(is->is_name_default) - 1,
           "myri%d", mx_num_instances);
  is->is_name = is->is_name_default;

  /* busbase is the value that can be passed to ioremap
     it might be different from both the PCI address,
     or the phys address or the kernel address
  */
  is->board_span = pci_resource_len(dev, 0);
  mx_bar64_reloc(dev);

  if (mx_pci_dev_base(dev, 0) == 0) {
    MX_WARN(("PCI BARs not accessible: base[0]=0x%lx\n",
	      mx_pci_dev_base(dev, 0)));
    status = -ENODEV;
    goto abort;
  }

  is->arch.pci_dev = dev;

  if (device == MX_PCI_DEVICE_Z8E && myri_mac) {
    char mac[18];
    mx_lz_read_mac(is, mac);
    if (mx_strcasecmp(myri_mac, mac) != 0) {
      MX_INFO(("NIC mac=%s ignored (driver option myri_mac=%s)\n", mac, myri_mac));
      status = -ENODEV;
      goto abort;
    }
  }

  mx_linux_pci_map_init(is);

  status = pci_set_dma_mask(dev, (dma_addr_t)~0ULL);
  if (status != 0) {
    MX_INFO(("64-bit pci address mask was refused, trying 32-bit"));
    status = pci_set_dma_mask(dev, (dma_addr_t)0xffffffffULL);
  }
  if (status != 0) {
    MX_PRINT(("Error %d setting DMA mask\n", status));
    goto abort;
  }

  if ((dev->device ==  MX_PCI_DEVICE_Z8E
       || dev->device == MX_PCI_DEVICE_Z8E_9)
      && dev->bus->self) {
    /* tweak upstream component of Myrinet pcie express devices */
    mx_pcie_bridge_conf(is, dev->bus->self);
  }
  
  if (dev->device ==  MX_PCI_DEVICE_Z8E
      || dev->device == MX_PCI_DEVICE_Z8E_9) {
    uint16_t ctl;
    pci_read_config_word(dev, MX_Z8_PCIE_DEVCTL, &ctl);
    ctl =  (ctl & ~MX_PCIE_DEVCTL_READRQ) | 0x5000; /* 4096 readrq */
    pci_write_config_word(dev, MX_Z8_PCIE_DEVCTL, ctl);
    pci_read_config_word(dev, MX_Z8_PCIE_DEVCTL, &ctl);
    if ((ctl & MX_PCIE_DEVCTL_READRQ) != 0x5000) {
      if (MX_RDMA_2K) {
	MX_INFO(("%s: pcie-devctl==0x%x (using RDMA-2K)\n",
		 is->is_name, ctl));
      } else {
	MX_WARN(("%s: Cannot set PCIE READRQ to 4096 (pcie-devctl==0x%x)\n",
		 is->is_name, ctl));
	goto abort;
      }
    }
  }

#if !MX_RDMA_FC && !MX_RDMA_PAD
  if (is->arch.is_unaligned && rev == 0) {
    if (myri_unaligned_force) {
      MX_WARN(("Using myri_unaligned_force: expect bad performance\n"));
    } else {
      MX_WARN(("This MX10G version does not support chipset E7520 with 10G-PCIE-8A-xxx\n"
	       "\tcontact help@myri.com (use myri_unaligned_force=1 to load anyway)\n"));
      goto abort;
    }
  }
#endif

#ifdef CONFIG_PCI_MSI
  if (mx_use_msi(is->arch.pci_dev)) {
    status = pci_enable_msi(is->arch.pci_dev);
    if (status != 0) {
      MX_INFO(("Error %d setting up MSIs, falling back to legacy interrupts\n",
	       status));
    } else {
      is->msi_enabled = 1;
    }
  }
#endif

  is->arch.irq = dev->irq;

  /*
  sprintf(is->arch.interrupt_string, "myri/myri%d", 
	   mx_num_instances);
  */
  sprintf(is->arch.interrupt_string, "myri%d", 
	   mx_num_instances);
  
  if (request_irq(is->arch.irq, (void *) mx_linux_intr,
		  IRQF_SHARED, is->arch.interrupt_string, is) == 0) {
    MX_INFO(("%s: allocated %s IRQ %d\n", 
	      is->is_name, is->msi_enabled ? "MSI" : "legacy", is->arch.irq));
  } else {
    MX_PRINT(("%s: Could not allocate %s IRQ %d\n", 
	      is->is_name, is->msi_enabled ? "MSI" : "legacy", is->arch.irq));
    status = -ENXIO;
    goto abort_with_msi;
  }

  /* setup write combining */
  mx_setup_writecomb(is);
  /* we don't need the last page on ZE NICs,
     and removing one reduce the possibility of something wierd happening
     with a Linux ioremap() bug (iounmap() unmaps one page too many, and
     whatever is after us might see their mapping attribute changed in the linear map */
  if (dev->device ==  MX_PCI_DEVICE_Z8E
      || dev->device == MX_PCI_DEVICE_Z8E_9) {
    int cap;

    is->board_span -= PAGE_SIZE;

    cap = pci_find_capability(is->arch.pci_dev, 9);

    if (myri_console && cap && !mx_cons.is) {
      mx_cons.is = is;
      mx_cons.linux_cons.flags |= CON_ENABLED;
      register_console(&mx_cons.linux_cons);
      MX_INFO(("Registered kernel console for NIC\n"));
      is->arch.console_cap = cap;
    }
  }

#if CONFIG_NUMA
  {
    int node = pcibus_to_node(is->arch.pci_dev->bus);
    if (node != -1)
      MX_INFO(("%s: memory affinity to NUMA node %d\n", is->is_name, node));
  }
#endif

  is->kernel_window = (void*)__get_free_page(GFP_KERNEL);
  if (!is->kernel_window) {
    MX_PRINT(("Failed to allocate kernel window\n"));
    status = -ENOMEM;
    goto abort_with_irq;
  }
  memset(is->kernel_window, 0, PAGE_SIZE);
  is->kernel_window->hz = HZ;
  mx_reserve_page(is->kernel_window);

  /* generic board initialization; load MCP and stuff */

  MX_LXX_INIT_WORK(&is->arch.intr_work, (void *) mx_do_bh_intr, is);
  spin_lock_init(&is->arch.intr_pending_lock);

  status = mx_instance_init(is, mx_num_instances);
  if (status != 0)  {
      MX_PRINT(("mx_instance_init failed\n"));
      goto abort_with_irq;
  }
  pci_set_drvdata(dev,is);

#if MYRI_SNF_KAGENT
  MX_LXX_INIT_DELAYED_WORK(&is->arch.snf.kagent_work, 
                           (void *) myri_snf_kagent_work, is);
#endif

  if (is->clk_initialized) {
    init_timer(&is->arch.clock_timer);
    is->arch.clock_timer.data = (unsigned long)is;
    is->arch.clock_timer.function = myri_linux_clock_timer;
    is->arch.clock_timer.expires = jiffies + (HZ * myri_clksync_period) / 1000;
    add_timer(&is->arch.clock_timer);
  }

  /*
   * prepend to list of MX devices
   */
  mx_devfs_register("myri", is, is->id*2);
  mx_devfs_register("myrip", is, is->id*2 + 1);
  MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT, 
		    ("mx_instance_init succeeded for unit %d\n",
		     mx_num_instances));
  mal_always_assert(mx_num_instances <= myri_max_instance);
  mx_mutex_exit(&is->sync);

  return 0;

  /* ERROR Handling */
 abort_with_irq:
  MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT, 
		    ("freeing irq %d\n", is->arch.irq));
  free_irq(is->arch.irq, is);
  
  mx_teardown_writecomb(is);

 abort_with_msi:
#ifdef CONFIG_PCI_MSI
  if (is->msi_enabled)
    pci_disable_msi(is->arch.pci_dev);
  is->msi_enabled = 0;
#endif
 abort:
  if (is && is->kernel_window) {
    mx_unreserve_page(is->kernel_window);
    free_page((unsigned long)is->kernel_window);
  }
  if (is == mx_cons.is) {
    unregister_console(&mx_cons.linux_cons);
    mx_cons.is = NULL;
  }
  if (is)
    mx_kfree(is);
  return status;
}

int mx_intr_recover(mx_instance_state_t *is)
{
#ifdef CONFIG_PCI_MSI
  if (is->msi_enabled) {
    free_irq(is->arch.irq, is);
    pci_disable_msi(is->arch.pci_dev);
    if (pci_enable_msi(is->arch.pci_dev) != 0) {
      MX_INFO(("Error resetting up MSIs\n"));
      return ENXIO;
    }
    is->arch.irq = is->arch.pci_dev->irq;
    if (request_irq(is->arch.irq, (void *) mx_linux_intr,
		    IRQF_SHARED, is->arch.interrupt_string, is) == 0) {
      MX_INFO(("reallocated MSI IRQ %d\n", is->arch.irq));
    } else {
      MX_INFO(("Error reallocating MSI\n"));
      return ENXIO;
    }
  }
#endif
  return 0;
}

static void
mx_linux_destroy_instance (mx_instance_state_t *is)
{
  mal_assert (is != NULL);
  mal_assert(mx_instances[is->id] == is);

  if (is->clk_initialized)
    del_timer_sync(&is->arch.clock_timer);

  mx_lxx_flush_scheduled_work();

#if LINUX_XX >= 26
  devfs_remove("myrip%d",is->id);
  devfs_remove("myri%d",is->id);
  if (myri_udev) {
    mx_class_device_destroy(mx_class, MKDEV(MX_MAJOR, is->id * 2));
    mx_class_device_destroy(mx_class, MKDEV(MX_MAJOR, is->id * 2 + 1));
  }
#else
  devfs_unregister(is->arch.devfs_handle[0]);
  devfs_unregister(is->arch.devfs_handle[1]);
#endif

  /* detach ethernet */
  mx_ether_detach(is);

  if (is->board_ops.disable_interrupt != NULL)
    is->board_ops.disable_interrupt(is);

  mx_teardown_writecomb(is);
  if (mx_instance_finalize(is) != 0) {
    MX_WARN(("Could not destroy instance, big problems in perspective\n"));
  }
  free_irq(is->arch.irq, is);
#ifdef CONFIG_PCI_MSI
  if (is->msi_enabled)
    pci_disable_msi(is->arch.pci_dev);
  is->msi_enabled = 0;
#endif
  mx_unreserve_page(is->kernel_window);
  free_page((unsigned long)is->kernel_window);
  if (is == mx_cons.is) {
    unregister_console(&mx_cons.linux_cons);
    mx_cons.is = NULL;
  }
  mx_kfree(is);
}

/****************************************************************
 * mx_linux_init_one
 *
 * Initializes one Myrinet card.  Called by the kernel pci
 *   scanning routines when the module is loaded.
 ****************************************************************/

static int
mx_linux_init_one (struct pci_dev *dev, const struct pci_device_id *ent)
{
  int status;
  int i;

  for (i=0; i < myri_bus_nb; i++) 
    if (dev->bus->number == myri_bus[i])
      break;
  if (myri_bus_nb && i == myri_bus_nb) {
    MX_INFO(("Ignoring Myri device %s because of myri_bus parameter\n", 
	     mx_pci_name(dev)));
    return -ENODEV;
  }
  if (mx_num_instances >= myri_max_instance) {
    MX_INFO(("Ignoring Myri device %s because myri_max_instance == %d\n", 
	     mx_pci_name(dev), myri_max_instance));
    return -ENODEV;
  } else if ((status = mx_linux_create_instance(dev)) == 0) {
    return 0;
  } else {
    MX_WARN(("Failed to initialize Myrinet Board at %s (%d)\n",
	      mx_pci_name(dev), status));
    return status;
  }
}

/****************************************************************
 * mx_linux_remove_one
 *
 * Does what is necessary to shutdown one Myrinet device. Called
 *   once for each Myrinet card by the kernel when a module is
 *   unloaded.
 ****************************************************************/

static void
mx_linux_remove_one (struct pci_dev *pdev)
{
  mx_instance_state_t *is;

  is = (mx_instance_state_t *) pci_get_drvdata(pdev);
  if (is != NULL) {
    mx_linux_destroy_instance(is);
  }
  pci_set_drvdata(pdev,0);
}


#define MX_PCI_DEVICE_MYRINET 0x8043
#define MX_PCI_VENDOR_MYRICOM 0x14c1

static struct pci_device_id mx_pci_tbl[] = {
  {MX_PCI_VENDOR_MYRICOM, MX_PCI_DEVICE_MYRINET,
     PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
  {MX_PCI_VENDOR_MYRICOM, MX_PCI_DEVICE_Z8E,
     PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
  {MX_PCI_VENDOR_MYRICOM, MX_PCI_DEVICE_Z8E_9,
     PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
  {0,},
};

static struct pci_driver myri_driver = {
  .name = MYRI_DRIVER_STR,
  .probe = mx_linux_init_one,
  .remove = mx_linux_remove_one,
  .id_table = mx_pci_tbl,
};

#if LINUX_XX >= 26
MODULE_DEVICE_TABLE(pci, mx_pci_tbl);
#endif

#if defined UTS_VERSION
static char mx_linux_uts_version[] = UTS_VERSION;
#else
static char mx_linux_uts_version[] = "";
#endif
static char mx_linux_uts_release[] = UTS_RELEASE;

/* Build a string in the driver to indicate the kernel version for
   which it was built.  The string should be in the form of a /bin/sh
   variable setting.  We will extract this string later using the
   "strings" program and install the driver at
   /lib/modules/<UTS_RELEASE>/kernel/drivers/net. */
char *MX_UTS_RELEASE = "MX_UTS_RELEASE=\"" UTS_RELEASE "\"";


static int
mx_watchdog_thread(void *unused)
{
  daemonize("myri_watchdog");
  while (!mx_module_is_exiting) {
    mx_watchdog_body();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    wait_event_interruptible_timeout(mx_watchdog_queue, 
		       mx_module_is_exiting, MX_WATCHDOG_TIMEOUT * HZ);
#else
    interruptible_sleep_on_timeout(&mx_watchdog_queue, MX_WATCHDOG_TIMEOUT * HZ);
#endif
    /* for obsolete kernels where daemonize does not  block all signals */
    flush_signals(current);
  }
  complete_and_exit(&mx_watchdog_completion, 0);
}

int
myri_snf_find_node_id(mx_endpt_state_t *es)
{
  int node = -1;

  if (es->node_id >= 0)
    return es->node_id;

  /* See if device is attached to specific node id */
  node = pcibus_to_node(es->is->arch.pci_dev->bus);
  if (node < 0)
    node = numa_node_id();

  return node;
}

#if MYRI_SNF_KAGENT
/* Currently, tests show that the work queue leaves more cpu availability for cases
 * where the user is not necessarily willing to make 1 core available for Sniffer.
 */
#define SNF_KAGENT_WORK_QUEUE 1

struct snf__rx;

static int
myri_snf_kagent_func(void *user)
{
  myri_snf_rx_state_t *snf = (myri_snf_rx_state_t *)user;

  mx_mutex_enter(&snf->sync);
  snf->kagent.state = MYRI_SNF_KAGENT_STARTED;
  mx_mutex_exit(&snf->sync);

  while (!kthread_should_stop())
    myri_snf_kagent_body(snf);
  return 0;
}


void
myri_snf_rx_wake(myri_snf_rx_state_t *snf, mx_endpt_state_t *es)
{
#if SNF_KAGENT_WORK_QUEUE
  if (snf->kagent.state != MYRI_SNF_KAGENT_DISABLED) {
    mx_instance_state_t *is = es->is;
    int cpun;
    
    cpun = is->arch.snf.kagent_wq_cpu_next;

    cpun = next_cpu(cpun, is->arch.snf.kagent_wq_cpumap);
    if (cpun == NR_CPUS)
      cpun = first_cpu(is->arch.snf.kagent_wq_cpumap);

    schedule_delayed_work_on(is->arch.snf.kagent_wq_cpu_next, 
                             &es->is->arch.snf.kagent_work, 0);
    is->arch.snf.kagent_wq_cpu_next = cpun;
  }
  else 
#endif
  {
    mx_wake(&es->wait_sync);
  }
}

static void
myri_snf_kagent_work(void *arg)
{
  mx_instance_state_t *is = 
    MX_WORK_STRUCT_ARG_CONTAINER_OF(arg, mx_instance_state_t, arch.snf.kagent_task);
  myri_snf_rx_state_t *snf = &is->snf;

  myri_snf_rx_kagent_progress(snf, 1);
}

int
myri_snf_kagent_init(myri_snf_rx_state_t *snf)
{
  mx_instance_state_t *is = snf->is;
  struct task_struct *task;

  if (!SNF_KAGENT_WORK_QUEUE) {
    task = kthread_create(myri_snf_kagent_func, snf, "myri_snf_rx%d", is->id);
    if (IS_ERR(task)) {
      MX_WARN(("%s: can't start kagent\n", is->is_name));
      return PTR_ERR(task);
    }
    else {
      wake_up_process(task);
      is->arch.snf.kagent_task = task;
    }
  }
  else {
    cpumask_t cpumap;

    mx_mutex_enter(&snf->sync);
    cpumap = cpu_online_map;

    if (snf->rx_ring_node_id >= 0) {
      /* Focus processing only on CPUs on that node id */
      myri_cpumask_and(&cpumap,
                       myri_node_to_cpumask(snf->rx_ring_node_id),
                       &cpu_online_map);
    }

    is->arch.snf.kagent_wq_cpumap = cpumap;
    is->arch.snf.kagent_wq_cpu_next = first_cpu(cpumap);
    snf->kagent.state = MYRI_SNF_KAGENT_STARTED;
    mx_mutex_exit(&snf->sync);
  }

  return 0;
}

int
myri_snf_kagent_fini(myri_snf_rx_state_t *snf)
{
  mx_instance_state_t *is = snf->is;
#if SNF_KAGENT_WORK_QUEUE
  cancel_delayed_work(&is->arch.snf.kagent_work);
  flush_scheduled_work();
#else
  if (is->arch.snf.kagent_task) {
    mx_wake(&snf->kagent.es->wait_sync);
    kthread_stop(is->arch.snf.kagent_task);
    is->arch.snf.kagent_task = NULL;
  }
#endif
  return 0;
}
#else /* !MYRI_SNF_KAGENT */

void
myri_snf_rx_wake(myri_snf_rx_state_t *snf, mx_endpt_state_t *es)
{
  mx_wake(&es->wait_sync);
}

#endif /* MYRI_SNF_KAGENT */

void
mx_cleanup_module (void)
{

  mx_module_is_exiting = 1;

  if (mx_linux_pci_driver_registered) {
#if MX_KERNEL_LIB
    myri_finalize_klib();
#endif
    
    wake_up(&mx_watchdog_queue);
    wait_for_completion(&mx_watchdog_completion);

    /* this call is responisible for tearing down each instance */
    pci_unregister_driver(&myri_driver);
  }
  (void) mx_finalize_driver();

#if (MAL_CPU_x86_64 || MAL_CPU_powerpc64) && !defined HAVE_COMPAT_IOCTL
  mx_linux_unregister_ioctl32();
#endif /* MAL_CPU_x86_64 || MAL_CPU_powerpc64 */

#if LINUX_XX >= 26
  mx_linux_class_fini();
  mx_linux_cdev_fini();
#else
  unregister_chrdev(MX_MAJOR, "myri");
#endif
#if MX_DMA_DBG
  if (myri_dma_pfn_max) {
    unsigned long i;
    unsigned long nbtables = (MX_NUM_BUS_PAGES+PAGE_SIZE -1) / PAGE_SIZE;
    for (i=0;i<nbtables;i++) {
      free_page((unsigned long)mx_dma_dbg_counts[i]);
    }
    kfree(mx_dma_dbg_counts);
  }
#endif

#if MAL_DEBUG  
  if (kmalloc_cnt == kfree_cnt && vmalloc_cnt == vfree_cnt)
    MAL_DEBUG_PRINT(MAL_DEBUG_MALLOC, ("No leaks\n"));
  else
    MAL_DEBUG_PRINT(MAL_DEBUG_MALLOC, 
		    ("Memory leak info:\n"
		     "\t kmallocs  = %d\n"
		     "\t vmallocs  = %d\n"
		     "\t kfrees    = %d\n"
		     "\t vfrees    = %d\n",
		     kmalloc_cnt, vmalloc_cnt, kfree_cnt, vfree_cnt));
#endif

  MX_INFO(("driver unloaded\n"));
  
}

int
mx_init_module (void)
{
  struct sysinfo mem_info;
#if MX_KERNEL_LIB
  int error;
#endif

  MX_INFO(("On %s, kernel version: %s %s\n", 
	    mx_current_utsname.machine,
	    mx_current_utsname.release,
	    mx_current_utsname.version));
  if (strcmp(mx_current_utsname.release, mx_linux_uts_release) != 0 ||
      strcmp(mx_current_utsname.version, mx_linux_uts_version) != 0) {
    MX_INFO(("module compiled with kernel headers of %s %s\n",
	      mx_linux_uts_release, mx_linux_uts_version));
  }
  
#if defined CONFIG_X86_PAE
  if (!cpu_has_pae || !(read_cr4() & X86_CR4_PAE)) {
      MX_WARN(("Trying to load PAE-enabled module on a non-PAE kernel\n"
		"please recompile appropriately\n"));
      return -ENODEV;
  }
#elif MAL_CPU_x86 
  if (cpu_has_pae && (read_cr4() & X86_CR4_PAE)) {
    MX_WARN(("Trying to load non-PAE module on a PAE-enabled kernel\n"
		"please recompile appropriately\n"));
    return -ENODEV;
  }
#endif

#if defined(CONFIG_SMP)
  MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT,
		    ("driver compiled with CONFIG_SMP enabled\n"));
#else
  /* XXX some way to check if kernel is SMP without looking at symbol table?*/
  MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT,
		    ("driver compiled without CONFIG_SMP enabled\n"));
#endif

  si_meminfo(&mem_info);
  mem_total_pages = mem_info.totalram;

  /*  this is sanity limits on registered memory to prevent the user
      from bringing the system to a complete stop */
  if (mem_total_pages >= (2 * MX_MAX_SAVE_FROM_PINNED)) {
    /* system with quite enough memory */
    unsigned long max_big;
    mx_max_user_pinned_pages_start = mem_total_pages - MX_MAX_SAVE_FROM_PINNED;
    max_big =	MX_MAX_USER_PINNED_BIGMEM(mem_total_pages);
    if (mx_max_user_pinned_pages_start > max_big)
      mx_max_user_pinned_pages_start = max_big;
  } else {
    mx_max_user_pinned_pages_start
      = MX_MAX_USER_PINNED_SMALLMEM(mem_total_pages);
  }

  mx_atomic_set(&myri_max_user_pinned_pages, mx_max_user_pinned_pages_start);

  MX_INFO(("Memory available for registration: %ld pages (%ld MBytes)\n",
	      mx_max_user_pinned_pages_start,
	      mx_max_user_pinned_pages_start >> (20 - PAGE_SHIFT)));

#if MX_DMA_DBG
  if (myri_dma_pfn_max) {
    unsigned long i;
    unsigned long nbtables = (MX_NUM_BUS_PAGES+PAGE_SIZE -1) / PAGE_SIZE;
    if (MX_NUM_BUS_PAGES > myri_dma_pfn_max) {
      MX_WARN(("--enable-dma-dbg: myri_dma_pfn_max must be >= %d here\n",
	       (unsigned)MX_NUM_BUS_PAGES));
      return -EIO;
    }
    mx_dma_dbg_counts = kmalloc(nbtables * sizeof(*mx_dma_dbg_counts), GFP_KERNEL);
    for (i=0;i<nbtables;i++) {
      mx_dma_dbg_counts[i] = (uint8_t*)__get_free_page(GFP_KERNEL);
      memset(mx_dma_dbg_counts[i],0,PAGE_SIZE);
    }
  }
#endif
#if LINUX_XX >= 26
  if (mx_linux_class_init())
    return -EBUSY;
  
  if (mx_linux_cdev_init()) {
    mx_linux_class_fini();
    MX_WARN(("register_chrdev failed (other myrinet driver loaded?)\n"));
    return -EBUSY;
  }
#else  

  if (register_chrdev(MX_MAJOR, "myri", &mx_fops) >= 0) {
    MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT,
		    ("mx: register_chrdev succeeded\n"));
  }
  else {
    MX_WARN(("register_chrdev failed (other myrinet driver loaded?)\n"));
    return -EBUSY;
  }
#endif /* linux 26 */

#if (MAL_CPU_x86_64 || MAL_CPU_powerpc64) && !defined HAVE_COMPAT_IOCTL
  if (mx_linux_register_ioctl32()) {
    MX_WARN(("Failed registering 32bit ioctls. 32bit apps might not work\n"));
  }
#endif /* MAL_CPU_x86_64 || MAL_CPU_powerpc64 */

#if defined(CONFIG_MTRR) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
  if (myri_pat_idx != 1 &&
      (myri_pat_idx > 7 || myri_pat_idx < 4)) {
    MX_WARN(("Illegal myri_pat_idx %d, disabling pat\n", myri_pat_idx));
    myri_pat_idx = -1;
  }
#endif
  
  /* from now we go through mx_cleanup_module in case of error */

  if (mx_init_driver() != 0) {
    MX_WARN(("Driver Initialization failure\n"));
    mx_cleanup_module();
    return -ENODEV;
  }

  if (mx_pci_module_init(&myri_driver) != 0) {
    mx_cleanup_module();
    MX_WARN(("No board initialized\n"));
    return -ENODEV;
  }

  mx_linux_pci_driver_registered = 1;
#if MX_10G_ENABLED
  MX_INFO(("%d board%s found and initialized\n",
	    mx_num_instances,
	    mx_num_instances == 1 ? "" : "s" ));
#else
  MX_INFO(("%d Myrinet 2G board%s found and initialized\n",
	    mx_num_instances,
	    mx_num_instances == 1 ? "" : "s" ));
#endif
  
  init_completion(&mx_watchdog_completion);
  init_waitqueue_head(&mx_watchdog_queue);
  if (kernel_thread(mx_watchdog_thread, 0, CLONE_FS | CLONE_FILES) < 0) {
    MX_WARN(("Cannot start the watchdog thread: No Parity recovery\n"));
    complete(&mx_watchdog_completion);
  }

#if MX_KERNEL_LIB
  /* initialize the kernel library */
  error = myri_init_klib();
  if (error) {
    MX_WARN(("Kernel Lib Initialization failure\n"));
    mx_cleanup_module();
    return -error;
  }
#endif

  return 0;
}

int
mx_alloc_dma_page_relax_order(mx_instance_state_t *is, char **addr, 
			      mx_page_pin_t *pin)
{
  int status;
  *addr = (char *)__get_free_pages(GFP_KERNEL, 0);
  if (!*addr)
    return ENOMEM;
#if MAL_DEBUG
  memset(*addr, -1, PAGE_SIZE);
#endif

  status = mx_dma_map(is, virt_to_page(*addr), &pin->dma);
  pin->relax_order = 1;
  if (status) {
    MX_PRINT_ONCE(("mx__alloc_dma_pages:pci_map failure: should be free=%d pages\n",
		   atomic_read(&is->arch.free_iommu_pages)));
    free_pages((unsigned long)*addr, 0);
    *addr = 0;
    return status;
  }
  pin->va = (uint64_t)(unsigned long)*addr;
  return 0;
}

int
mx_alloc_dma_page(mx_instance_state_t *is, char **addr, mx_page_pin_t *pin)
{
  {
  dma_addr_t dma;
  void * p;

  p = mx_dma_alloc_coherent(is->arch.pci_dev, PAGE_SIZE, &dma, GFP_KERNEL);
  if (!p)
    return ENOMEM;
  pin->dma.high = (uint64_t)dma >> 32;
  pin->dma.low = dma;
  pin->va = (uintptr_t)p;
  pin->relax_order = 0;
  *addr = p;
  if (myri_dma_pfn_max)
    mx_dma_bitmap_add(is, dma);
  return 0;
  }
}

void
mx_free_dma_page(mx_instance_state_t *is, char **addr, mx_page_pin_t *pin)
{
  if (pin->relax_order == 0) {
    dma_addr_t dma = ((uint64_t)pin->dma.high << 32) + pin->dma.low;
    if (myri_dma_pfn_max)
      mx_dma_bitmap_remove(is, dma);
    mx_dma_free_coherent(is->arch.pci_dev, PAGE_SIZE, *addr, dma);
    *addr = 0;
    return;
  }
  mx_dma_unmap(is, &pin->dma);
  free_pages((unsigned long)*addr, 0);
  if (MAL_DEBUG)
    *addr = 0;
}

int
mx_optimized_alloc_copyblock(mx_instance_state_t *is, 
			     mx_copyblock_t *cb,
			     int relax_order)
{
  char *va;
  int i, status;
  unsigned npages = (cb->size + PAGE_SIZE - 1) / PAGE_SIZE;
  unsigned nvpages = (cb->size + MX_VPAGE_SIZE - 1) / MX_VPAGE_SIZE;

  cb->pins = mx_kmalloc(sizeof(cb->pins[0]) * npages, MX_MZERO|MX_WAITOK);
  cb->dmas = mx_kmalloc(sizeof(mcp_dma_addr_t) * nvpages, MX_MZERO|MX_WAITOK);
  if ((cb->pins == NULL) ||(cb->dmas == NULL)) {
    MX_WARN(("copyblock allocation failed due to lack of memory\n"));
    status = ENOMEM;
    goto abort;
  }
 
  for (i = 0; i < npages; i++) {
    if (relax_order)
      status = mx_alloc_zeroed_dma_page_relax_order(is, &va, &cb->pins[i]);
    else
      status = mx_alloc_zeroed_dma_page(is, &va, &cb->pins[i]);
    if (status)
      goto abort;
    mx_reserve_page((void *)va);
  }

  for (i = 0; i < nvpages; i++) {
    cb->dmas[i].high = htonl(cb->pins[i/(PAGE_SIZE/MX_VPAGE_SIZE)].dma.high);
    cb->dmas[i].low = htonl(cb->pins[i/(PAGE_SIZE/MX_VPAGE_SIZE)].dma.low);
  }

  return 0;

 abort:
  mx_optimized_free_copyblock(is, cb);
  return status;
}

uint16_t
mx_bridge_pci_sec_status(mx_instance_state_t *is)
{
  uint16_t pci_status = -1;
  struct pci_dev *bridge;
  
  if (!is || !is->arch.pci_dev || !is->arch.pci_dev->bus->self)
    return 0;
  bridge = is->arch.pci_dev->bus->self;
  
  if (pci_read_config_word(bridge, PCI_SEC_STATUS, &pci_status)) {
    MX_WARN(("Error while reading PCI sec status on %s\n", mx_pci_name(bridge)));
  }
  return pci_status;
}

uint32_t
mx_bridge_id(mx_instance_state_t *is)
{
  uint32_t id;
  struct pci_dev *bridge;
  
  if (!is || !is->arch.pci_dev || !is->arch.pci_dev->bus->self)
    return 0;
  bridge = is->arch.pci_dev->bus->self;
  pci_read_config_dword(bridge, PCI_VENDOR_ID, &id);
  return id;
}

void
mx_optimized_free_copyblock(mx_instance_state_t *is, mx_copyblock_t *cb)
{
  char *va;
  int i;
  unsigned npages = (cb->size + PAGE_SIZE - 1) / PAGE_SIZE;

  if (cb->pins != NULL) {
    for (i = 0; i < npages; i++) {
      va = (char *)(unsigned long)cb->pins[i].va;
      if (va != 0) {
	mx_unreserve_page(va);
	mx_free_dma_page(is, &va, &cb->pins[i]);
      }
    }
    mx_kfree(cb->pins);
    cb->pins = NULL;
  }
  
  if (cb->dmas != NULL) {
    mx_kfree(cb->dmas);
    cb->dmas = NULL;
  }
}

void *
mx_map_huge_copyblock(mx_huge_copyblock_t *hcb)
{
  struct page **pages = NULL;
  unsigned long i;
  void *kva = NULL;

  pages = mx_kmalloc(sizeof(struct page *) * hcb->pinned_pages, MX_WAITOK);
  if (pages == NULL)
    return NULL;

  for (i = 0; i < hcb->pinned_pages; i++)
    pages[i] = virt_to_page(hcb->pins[i].va);

#if 1
  if ((hcb->pinned_pages << PAGE_SHIFT) > (~0U)) {
    /* Bug in Linux vmap, casts pages as an unsigned int */
    printk("too many pages for vmap!\n");
    goto fail;
  }

  kva = vmap(pages, hcb->pinned_pages, VM_MAP, PAGE_KERNEL);
#else
  {
    struct vm_struct *area;
    area = get_vm_area((hcb->pinned_pages << PAGE_SHIFT), VM_MAP);
    if (area == NULL)
      goto fail;

    if (map_vm_area(area, PAGE_KERNEL, &pages))
      vunmap(area->addr);
    else
      kva = area->addr;
  }
#endif

fail:
  mx_kfree(pages);
  hcb->kva = kva;
  return kva;
}

void
mx_unmap_huge_copyblock(mx_huge_copyblock_t *hcb)
{
  if (hcb->kva) {
    vunmap(hcb->kva);
    hcb->kva = NULL;
  }
}

/* Copy function from/to user space in ANOTHER process */

int
mx_copy_to_user_mm(uaddr_t udst, const void * ksrc, struct mm_struct *dst_mm,
		   uint32_t len)
{
  /* this code is inspired from access_process_vm */
  struct page *page;
  int status;

  while (len > 0) {
    uint32_t offset = udst & ~PAGE_MASK;
    uint32_t chunk = MIN(len, PAGE_SIZE - offset);
    char *kdst;

    /* Using "current" rather than "p" is not a typo.  It is done so
       that page faults are charged to the caller.
       Besides, task p might disappear at any time */
    mx_mmap_down_write(dst_mm);
    status = get_user_pages(current, dst_mm, udst, 1, 1, 0, &page, NULL);
    mx_mmap_up_write(dst_mm);
    if (status != 1) {
      return -status;
    }
    // kdst = kmap_atomic(page, KM_USER0);
    kdst = kmap_atomic(page);
    /* copy_to_user_page should be used instead. But it uses some symbols that
     * are not exported on some architectures...
     */
    memcpy(kdst+offset, ksrc, chunk);
    set_page_dirty_lock(page);
    // kunmap_atomic(kdst, KM_USER0);
    kunmap_atomic(kdst);
    put_page(page);
    ksrc += chunk;
    udst += chunk;
    len -= chunk;
  }

  return 0;
}

int
mx_copy_from_user_mm(char *kdst, uaddr_t usrc, struct mm_struct *src_mm,
		     uint32_t len)
{
  /* this code is inspired from access_process_vm */
  struct page *page;
  int status;

  while (len > 0) {
    uint32_t offset = usrc & ~PAGE_MASK;
    uint32_t chunk = MIN(len, PAGE_SIZE - offset);
    char *ksrc;

    /* Using "current" rather than "p" is not a typo.  It is done so
       that page faults are charged to the caller.
       Besides, task p might disappear at any time */
    mx_mmap_down_write(src_mm);
    status = get_user_pages(current, src_mm, usrc, 1, 0, 0, &page, NULL);
    mx_mmap_up_write(src_mm);
    if (status != 1) {
      return -status;
    }
    // ksrc = kmap_atomic(page, KM_USER0);
    ksrc = kmap_atomic(page);
    /* copy_from_user_page should be used instead. But it uses some symbols that
     * are not exported on some architectures...
     */
    memcpy(kdst, ksrc+offset, chunk);
    // kunmap_atomic(ksrc, KM_USER0);
    kunmap_atomic(ksrc);
    put_page(page);
    usrc += chunk;
    kdst += chunk;
    len -= chunk;
  }

  return 0;
}

/* OS specific callback for direct get, copying from another process
 * user-space to current process user-space.
 */
int
mx_arch_copy_user_to_user(uaddr_t udst, uaddr_t usrc, void * src_space,
			  uint32_t len)
{
  struct mm_struct * mm_src = (struct mm_struct *) src_space;
  struct page *page;
  int status;

  while (len > 0) {
    uint32_t offset = usrc & ~PAGE_MASK;
    uint32_t chunk = MIN(len, PAGE_SIZE - offset);
    char *ksrc;

    /* Using "current" rather than "p" is not a typo.  It is done so
       that page faults are charged to the caller.
       Besides, task p might disappear at any time */
    mx_mmap_down_write(mm_src);
    status = get_user_pages(current, mm_src, usrc, 1, 0, 0, &page, NULL);
    mx_mmap_up_write(mm_src);
    if (status != 1) {
      return -status;
    }
    ksrc = kmap(page);
    status = copy_to_user((void*)udst, ksrc + offset, chunk);
    kunmap(page);
    put_page(page);
    if (status) {
      return EFAULT;
    }
    usrc += chunk;
    udst += chunk;
    len -= chunk;
  }

  return 0;
}

int
mx_shmem_thread_handle(struct mx_endpt_state *es, uint32_t val)
{
  if (val == 1 && es->arch.shm_exit == 0) {
    es->arch.mm = current->mm;
    wake_up(&es->arch.shm_waitq);
    wait_event_interruptible(es->arch.shm_waitq, es->arch.shm_exit);
    down(&es->arch.shm_lock);
    es->arch.mm = NULL;
    up(&es->arch.shm_lock);
  } else if (val == 0) {
    es->arch.shm_exit = 1;
    wake_up(&es->arch.shm_waitq);
  } else if (val == 2) {
    /* use in mx_open_endpoint, to make sure dedicated thread (with
       val == 0) has reached this function  */
    wait_event_interruptible(es->arch.shm_waitq, es->arch.mm);
  }
  return 0;
}

int
mx_direct_get(mx_endpt_state_t *dst_es, mx_shm_seg_t *dst_segs, uint32_t dst_nsegs,
	      mx_endpt_state_t *src_es, mx_shm_seg_t *src_segs, uint32_t src_nsegs,
	      uint32_t length)
{
  struct mm_struct *mm_src;
  int status = 0;

  mx_mutex_enter(&src_es->sync);
  down(&src_es->arch.shm_lock);
  mm_src = src_es->arch.mm;
  if (!mm_src) {
    up(&src_es->arch.shm_lock);
    mx_mutex_exit(&src_es->sync);
    return ESRCH;
  }
  mx_mutex_exit(&src_es->sync);

  /* get destination segments from current mm */
  if (dst_nsegs > 1) {
    uaddr_t uptr = dst_segs[0].vaddr;
    dst_segs = mx_kmalloc(dst_nsegs * sizeof(*dst_segs), 0);
    if (!dst_segs) {
      status = ENOMEM;
      goto abort;
    }
    status = copy_from_user(dst_segs, (void*) uptr, dst_nsegs * sizeof(*dst_segs));
    if (status) {
      status = -status;
      goto abort_with_dst_segs;
    }
  }

  /* get source segments from another mm */
  if (src_nsegs > 1) {
    uaddr_t uptr = src_segs[0].vaddr;

    src_segs = mx_kmalloc(src_nsegs * sizeof(*src_segs), 0);
    if (!src_segs) {
      status = ENOMEM;
      goto abort_with_dst_segs;
    }

    status = mx_copy_from_user_mm((char *) src_segs, uptr, mm_src,
				  src_nsegs * sizeof(*src_segs));
    if (status) {
      goto abort_with_src_segs;
    }
  }

  status = mx_direct_get_common(dst_segs, dst_nsegs,
				mm_src, src_segs, src_nsegs,
				length);

 abort_with_src_segs:
  if (src_nsegs > 1)
    mx_kfree (src_segs);
 abort_with_dst_segs:
  if (dst_nsegs > 1)
    mx_kfree (dst_segs);
 abort:
  up(&src_es->arch.shm_lock);
  return status;
}


/****************************************************************
 * PCI config space functions
 ****************************************************************/

#define pcibios_to_mx(rw, size, linuxname, c_type, star)           \
int                                                                  \
mx_##rw##_pci_config_##size (mx_instance_state_t *is,            \
                               uint32_t offset,                      \
                               uint##size##_t star value)            \
{                                                                    \
  mal_assert (is);                                                  \
  mal_assert (is->arch.pci_dev);                                    \
  return (pci_##rw##_config_##linuxname (is->arch.pci_dev,           \
                                          (unsigned char) offset,    \
                                          (c_type star) value));     \
}

pcibios_to_mx (read, 32, dword, unsigned int, *);
pcibios_to_mx (write, 32, dword, unsigned int,);
pcibios_to_mx (read, 16, word, unsigned short, *);
pcibios_to_mx (write, 16, word, unsigned short,);
pcibios_to_mx (read, 8, byte, unsigned char, *);
pcibios_to_mx (write, 8, byte, unsigned char,);

int
mx_pcie_link_reset(mx_instance_state_t *is)
{
  struct pci_dev *bridge = is->arch.pci_dev->bus->self;
  unsigned cap;
  uint8_t link_ctl;
  if (!bridge 
      || (!(cap = pci_find_capability(bridge, PCI_CAP_ID_EXP)))
      || pci_read_config_byte(bridge, cap + PCI_EXP_LNKCTL, &link_ctl) < 0) {
    MX_INFO(("NIC at %s has no PCIE upstream bridge !!\n", mx_pci_name(is->arch.pci_dev)));
    return -1;
  }
  pci_write_config_byte(bridge, cap+PCI_EXP_LNKCTL, link_ctl | PCI_EXP_LNKCTL_DISABLE);
  mx_spin(200000);
  pci_write_config_byte(bridge, cap+PCI_EXP_LNKCTL, link_ctl);
  return 0;
}

void
mx_set_default_hostname(void)
{
  strncpy(mx_default_hostname, mx_current_utsname.nodename, sizeof(mx_default_hostname) - 1);
  mx_default_hostname[sizeof(mx_default_hostname) - 1] = '\0';
}



static void
mx_console_write(struct console *c, const char *buf, unsigned count)
{
  mx_instance_state_t *is = mx_cons.is;
  struct pci_dev *pdev = is->arch.pci_dev;
  unsigned cap_vs = is->arch.console_cap;
  u32 val;
  u8 * bytes = (u8*)buf;

  while (count >= 4) {
    val = bytes[0] + (bytes[1] << 8) + (bytes[2] << 16) + (bytes[3] << 24);
    pci_write_config_dword(pdev, cap_vs + 0x1c, val);
    count -= 4;
    bytes += 4;
  }
  if (count >= 2) {
    val = bytes[0] + (bytes[1] << 8);
    pci_write_config_word(pdev, cap_vs + 0x1c, val);
    count -= 2;
    bytes += 2;
  }
  if (count) {
    val = bytes[0];
    pci_write_config_byte(pdev, cap_vs + 0x1c, val);
  }
}


module_init (mx_init_module);
module_exit (mx_cleanup_module);

#include "mx_old_kernels.c"
