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

#ifndef _mx_dma_map_h_
#define _mx_dma_map_h_

#include <asm/atomic.h>

/* Powerpc64 returns a -1 for error when using pci_map_single (0 might
   be a valid dma address) Every other architecture either uses 0 or
   never fails
*/

#if MAL_CPU_powerpc64  
#define INVALID_DMA_ADDR ((dma_addr_t)-1)
#else
#define INVALID_DMA_ADDR ((dma_addr_t)0)
#endif


#if MX_DMA_DBG
uint8_t *mx_get_lock_count(unsigned long pfn);
void check_bitmap_word(mx_instance_state_t *is, unsigned long addr, int line);
/* debugging the dma-dbg code is disabled by default,
   if something sounds fishy in the dma-dbg code itself
   activate check_bitmap-word by commenting the above line */
#define check_bitmap_word(i,a,l)
#define mx_dma_dbg_lock_kernel() lock_kernel()
#define mx_dma_dbg_unlock_kernel() unlock_kernel()
#define MX_DMA_DBG_BITS (1 + MX_2K_PAGES * 2)
#else
#define mx_dma_dbg_lock_kernel()
#define mx_dma_dbg_unlock_kernel()
#endif

static inline void
mx_dma_bitmap_add(mx_instance_state_t *is, dma_addr_t bus)
{
#if MX_DMA_DBG
  uint8_t *countp;
  unsigned mod;
  uint64_t word;

  word = bus - is->lanai.dma_dbg_lowest_addr;
  word /= PAGE_SIZE;
  word *= (1 + MX_2K_PAGES);
  mal_always_assert(word < myri_dma_pfn_max);
  mod = word & 31;
  word /= 32;

  countp = mx_get_lock_count((bus-is->lanai.dma_dbg_lowest_addr)/PAGE_SIZE);
  check_bitmap_word(is, bus - is->lanai.dma_dbg_lowest_addr, __LINE__);

  if (!*countp) {
    uint32_t val;
    val = is->dma_dbg_bitmap_host[word];
    mal_always_assert(!(val & (MX_DMA_DBG_BITS<<mod)));
    val |= MX_DMA_DBG_BITS<<mod;
    is->lanai.dma_dbg_bitmap[word] = htonl(val);
    is->dma_dbg_bitmap_host[word] = val;
    mb();
  }
  *countp += 1;
  mal_assert(*countp <= 127);
  check_bitmap_word(is, bus - is->lanai.dma_dbg_lowest_addr, __LINE__);
#endif
}

static inline int
mx_dma_map(mx_instance_state_t *is, struct page *page, mcp_dma_addr_t *dma)
{
  dma_addr_t bus;

  if (!is->arch.has_iommu || atomic_read (&is->arch.free_iommu_pages) > 0) {
    bus = pci_map_page (is->arch.pci_dev, page, 0, PAGE_SIZE,
			PCI_DMA_BIDIRECTIONAL);
    if (is->arch.has_iommu && bus != INVALID_DMA_ADDR) {
#if MX_OPTERON_IOMMU
      if (is->arch.aper_base && (bus < is->arch.aper_base
				 || bus >= is->arch.aper_base + is->arch.aper_size)) {
	MX_PRINT_ONCE(("iommu mapping outside of aperture: 0x%llx\n", bus));
	atomic_inc (&is->arch.free_iommu_pages);
      }
#endif
      atomic_dec (&is->arch.free_iommu_pages);
    }
  } else {
    MX_PRINT_ONCE(("Warning: iommu allocation exhausted\n"));
#if MX_OPTERON_IOMMU
    bus = __pa(page_address(page));
#else
    return ENOMEM;
#endif
  }
  if (bus == INVALID_DMA_ADDR) {
    MX_PRINT_ONCE(("mx_dma_map:pci_map failure: should be free=%d pages\n",
		   atomic_read(&is->arch.free_iommu_pages)));
    return ENXIO;
  } 
  dma->low = MX_LOWPART_TO_U32(bus);
  dma->high = MX_HIGHPART_TO_U32(bus);
  if (myri_dma_pfn_max)
    mx_dma_bitmap_add(is, bus);
  return 0;
}

static inline void
mx_dma_bitmap_remove(mx_instance_state_t *is, dma_addr_t bus)
{
#if MX_DMA_DBG
  uint8_t *countp;
  unsigned mod;
  uint64_t word;

  countp = mx_get_lock_count((bus-is->lanai.dma_dbg_lowest_addr)/PAGE_SIZE);

  word = bus - is->lanai.dma_dbg_lowest_addr;
  word /= PAGE_SIZE;
  word *= (1 + MX_2K_PAGES);
  mal_always_assert(word < myri_dma_pfn_max);
  mod = word & 31;
  word /= 32;

  mal_assert(*countp <= 127 && *countp > 0);
  check_bitmap_word(is, bus - is->lanai.dma_dbg_lowest_addr, __LINE__);
  *countp -= 1;
  if (!*countp && is->dma_dbg_bitmap_host) {
    uint32_t val;
    val = is->dma_dbg_bitmap_host[word];
    mal_always_assert(val & (MX_DMA_DBG_BITS<<mod));
    val &= ~(MX_DMA_DBG_BITS<<mod);
    is->dma_dbg_bitmap_host[word] = val;
    is->lanai.dma_dbg_bitmap[word] = htonl(val);
    mb();
  }
  check_bitmap_word(is, bus - is->lanai.dma_dbg_lowest_addr, __LINE__);
#endif
}

static inline void
mx_dma_unmap (mx_instance_state_t *is, mcp_dma_addr_t *dma)
{
  
  dma_addr_t bus;
  
  bus = (dma_addr_t)dma->low;
  bus |=  MX_U32_TO_HIGHPART(dma->high, bus);
  
  if (myri_dma_pfn_max)
    mx_dma_bitmap_remove(is, bus);

  if (is->arch.has_iommu) {
#if MX_OPTERON_IOMMU
    if (is->arch.aper_base && (bus < is->arch.aper_base
			       || bus >= is->arch.aper_base + is->arch.aper_size)) {
      MX_PRINT_ONCE(("Removing direct iommu mapping 0x%llx\n", bus));
      return;
    }
#endif
    atomic_inc(&is->arch.free_iommu_pages);
  }
  pci_unmap_page(is->arch.pci_dev,(dma_addr_t) bus, PAGE_SIZE,
		 PCI_DMA_BIDIRECTIONAL);
}

#endif /* _mx_dma_map_h_ */
