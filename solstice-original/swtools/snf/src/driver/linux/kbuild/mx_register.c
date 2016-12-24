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
 * Copyright 2003 - 2008 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#include "mx_arch.h"
#include "mx_misc.h"
#include "mx_instance.h"
#include "mx_malloc.h"
#include "mx_pio.h"


#define MX_CRC32_ACCUM(accum, in)					\
  do {									\
    accum = (accum << 4)						\
      ^ (is->crc32_table[(((accum >> 28)				\
			   ^ (in >> ((sizeof(in) * 8) - 4))) & 15)]);	\
    in = in << 4;							\
  } while (0)


static
void
mx_crc32_init(mx_instance_state_t *is)
{
  uint32_t i, j, accum;
  
  /* initialize the 4-bit CRC table (it fits in a 64 Bytes cache line) */
  for (i = 0; i < 16; i++) {
    accum = i << 28;
    for (j = 0; j < 4; j++)
      accum = (accum << 1) ^ (((accum >> 31) & 0x1) ? 0x04c11db7 : 0);
    
    is->crc32_table[i] = accum;
  }
}

static inline
void
mx_crc32_u32(mx_instance_state_t *is, uint32_t *crc, uint32_t val)
{
  uint32_t i;
  
  for (i = 0; i < 8; i++)
    MX_CRC32_ACCUM((*crc), val);
}

static inline
void
x_crc32_u16(mx_instance_state_t *is, uint32_t *crc, uint16_t val)
{
  uint32_t i;
  
  for (i = 0; i < 4; i++)
    MX_CRC32_ACCUM((*crc), val);
}

#if 0

static
mx_mmu_t *
mx_mmu_get(mx_endpt_state_t *es, uint64_t va)
{
  uint32_t key_high, key_low, crc, i;
  mx_mmu_t *mmu;
  mx_instance_state_t *is = es->is;
  
  /* compute the hash key */
  key = va & MX_MMU_VA_MASK;
  key += MX_MMT_HASH_VALID + (es->endpt << 16) + rdmawin_id;
  
  /* compute the hash */
  crc = 0;
  mx_crc32_u32(is, &crc, MX_HIGHPART_TO_U32(key));
  mx_crc32_u32(is, &crc, MX_LOWPART_TO_U32(key));
  i = crc & (MX_MMU_HASH - 1);
  mmu = is->mmu.host_hash[i];
  
  if (mmu == 0)
    goto miss;
  
  do {
    /* check if we have a hit */
    if (mmu->key == key)
      return meta;
    
    /* next in hash list */
    mmu = mmu->next;
  } while (mmu);
  
 miss: 
  /* allocate a mmu object */
  if (is->mmu.free == 0)
    return 0;
  mmu = is->mmu.free;
  is->mmu.free = mmu->next;
  
  /* insert in the host hash */
  meta->next = is->mmu.host_meta_hash[i];
  is->mmu.host_meta_hash[i] = meta;
  meta->key = key;
  
  /* insert in the mcp hash */
  mal_assert(MX_PIO_READ(meta->mcp.key_low) == 0);
  MX_PIO_WRITE(&is->mmt.mcp_hash[i].dma.high, htonl(pgd->pin.dma.high));
  MX_PIO_WRITE(&is->mmt.mcp_hash[i].dma.low, htonl(pgd->pin.dma.low));
  MX_PIO_WRITE(&is->mmt.mcp_hash[i].key_high, htonl(key_high));
  MAL_STBAR();
  MX_PIO_WRITE(&is->mmt.mcp_hash[i].key_low, htonl(key_low));
  MAL_STBAR();
  MX_PIO_WRITE(&is->mmt.mcp_hash[i] = mcp_mmu_meta_index 
  
  
  return meta;
}


static
mx_mmt_pgd_t *
mx_mmt_alloc_pgd(mx_instance_state_t *is)
{
  mx_mmt_pgd_t *pgd;
  uint32_t i;
  
  /* allocate the new pgd */
  pgd = mx_kmalloc(sizeof(mx_mmt_pgd_t), MX_WAITOK | MX_MZERO);
  if (!pgd) 
    return 0;
  
  /* allocate the metadata page accessible by the NIC */
  if (mx_alloc_dma_page(is, (char **)&pgd->meta, &pgd->pin)) {
    mx_kfree(pgd);
    return 0;
  }
  
  /* initialize the metadata page */
  for (i = 0; i < MX_ADDRS_PER_VPAGE; i++)
    pgd->meta[i].low = MX_DMA_INVALID_ENTRY;
  
  return pgd;
}

static
mx_mmt_pmd_t *
mx_mmt_alloc_pmd(mx_instance_state_t *is, uint64_t va)
{
  mx_mmt_pmd_t *pmd;
  uint32_t i;
  
  mal_assert(MX_MMT_PMD_OFFSET(va) == 0);
  
  /* allocate the new pmd */
  pmd = mx_kmalloc(sizeof(mx_mmt_pmd_t), MX_WAITOK | MX_MZERO);
  if (!pmd) 
    return 0;
  
  /* allocate the metadata page accessible by the NIC */
  if (mx_alloc_dma_page(is, (char **)&pmd->meta, &pmd->pin)) {
    mx_kfree(pmd);
    return 0;
  }
  
  /* initialize the metadata page and the pin table */
  for (i = 0; i < MX_ADDRS_PER_VPAGE; i++) {
    pmd->meta[i].low = MX_DMA_INVALID_ENTRY;
    pmd->pins[i].va = va;
    pmd->pins[i].dma.low = MX_DMA_INVALID_ENTRY;
    va += MX_VPAGE_SIZE;
  }
  
  return pmd;
}


static
mx_mmt_pgd_t *
mx_mmt_get_pgd(mx_endpt_state_t *es, uint64_t va, uint32_t allocate)
{
  uint32_t key_high, key_low, crc, i;
  mx_mmt_hash_t *hash;
  mx_mmt_pgd_t *pgd;
  mx_instance_state_t *is = es->is;
  
  /* compute the hash key */
  va &= MX_MMT_PGD_MASK;
  key_high = MX_HIGHPART_TO_U32(va);
  key_low = MX_LOWPART_TO_U32(va) + es->endpt + MX_MMT_HASH_VALID;

  /* compute the hash */
  crc = 0;
  mx_crc32_u32(is, &crc, key_high);
  mx_crc32_u32(is, &crc, key_low);
  i = crc & MX_MMT_HASH_MASK;
  hash = &is->mmt.host_hash[i];
  
  do {
    /* check if we have a hit */
    if ((hash->key_high == key_high) && (hash->key_low == key_low))
      goto hit;
    
    /* check if we have a miss */
    if (hash->key_low == 0)
      goto miss;
    
    i = (i + 1) & MX_MMT_HASH_MASK;
    hash = &is->mmt.host_hash[i];
  } while (1);
  
 miss:
  if (!allocate)
    return 0;
  
  /* allocate a pgd */
  pgd = mx_mmt_alloc_pgd(is);
  mx_mem_check(pgd);
  
  /* insert in the host MMT hash */
  mal_assert(hash->key_low == 0);
  hash->key_high = key_high;
  hash->key_low = key_low;
  hash->pgd = pgd;
  
  /* insert in the mcp MMT hash */
  mal_assert(MX_PIO_READ(&is->mmt.mcp_hash[i].key_low) == 0);
  MX_PIO_WRITE(&is->mmt.mcp_hash[i].dma.high, htonl(pgd->pin.dma.high));
  MX_PIO_WRITE(&is->mmt.mcp_hash[i].dma.low, htonl(pgd->pin.dma.low));
  MX_PIO_WRITE(&is->mmt.mcp_hash[i].key_high, htonl(key_high));
  MAL_STBAR();
  MX_PIO_WRITE(&is->mmt.mcp_hash[i].key_low, htonl(key_low));
  
 hit:
  return hash->pgd;
  
 handle_enomem:
  return 0;
}

static
mx_mmt_pmd_t *
mx_mmt_get_pmd(mx_endpt_state_t *es, mx_mmt_pgd_t *pgd, uint64_t va, 
	       uint32_t allocate)
{
  uint32_t i;
  mx_mmt_pmd_t *pmd;
  
  /* compute the hash key */
  i = MX_MMT_PGD_OFFSET(va) / MX_MMT_PMD_SIZE;
  mal_assert(i < MX_ADDRS_PER_VPAGE);
  pmd = pgd->pmds[i];
  
  if ((pmd == 0) && allocate) {
    pmd = mx_mmt_alloc_pmd(es->is, va & MX_MMT_PMD_MASK);
    mx_mem_check(pmd);
    
    pgd->pmds[i] = pmd;
    pgd->meta[i].high = pmd->pin.dma.high;
    MAL_STBAR();
    pgd->meta[i].low = pmd->pin.dma.low;
  }
  
  return pmd;
  
 handle_enomem:
  return 0;
}

int
mx_mmt_register(mx_endpt_state_t *es, mx_reg_seg_t *usegs, uint32_t num_usegs, uintptr_t memory_context)
{
  uint32_t vpages, vpages_start, pgd_vpages, pmd_vpages, flags, i;
  uint64_t va, va_start;
  mx_mmt_pgd_t *pgd;
  mx_mmt_pmd_t *pmd;
  int status;

  /* user endpoint may only use user memory (and should be passed */
  if (!es->is_kernel) {
    if (memory_context != MX_PIN_UNDEFINED) {
      MX_WARN(("mx_register: user memory context should be undefined\n"));
      return EINVAL;
    }
    memory_context = mx_get_memory_context();
  }

  if (memory_context & MX_PIN_FULLPAGES) {
    /* multiple fullpage segments */
    vpages = 0;
    for(i = 0; i < num_usegs; i++) {
      if (MX_VPAGE_OFFSET(usegs[i].vaddr) || (MX_VPAGE_OFFSET(usegs[i].len))) {
	MX_WARN(("mx_register: fullpage segments must be single pages\n"));
	return EINVAL;
      }
      vpages += usegs[i].len / MX_VPAGE_SIZE;
      MX_WARN(("mx_register: FIXME\n"));
      return EINVAL;
    }
  } else {
    /* only one segment is supported in the general case */
    if (num_usegs > 1) {
      MX_WARN(("mx_register: scatter/gather is not yet supported \n"));
      return EINVAL;
    }

    /* use the first segment */
    vpages = usegs[0].len / MX_VPAGE_SIZE;
    if (MX_VPAGE_OFFSET(usegs[0].vaddr) || MX_VPAGE_OFFSET(usegs[0].len)) {
      MX_WARN(("mx_register: registration must use page boundaries\n"));
      return EINVAL;
    }
  }

  if (vpages == 0)
    return 0;

  /* grab the es mutex, so as to ensure that no other thread is racing
     to register something on this handle.  This is theoretically not
     needed, but is here to protect us from malicous users */
  mx_mutex_enter(&es->sync);
  
  flags = MX_PIN_STREAMING | MX_PIN_CTX_TO_TYPE(memory_context);
  va = va_start = usegs[0].vaddr;
  vpages_start = vpages;

  /* walk the region, pinning pages as we go and storing the DMA address
   the metadata pages */
  do {
    /* get the pgd and allocate it if needed */
    pgd = mx_mmt_get_pgd(es, va, 1);
    if (!pgd) {
      MX_WARN(("cannot allocate pgd\n"));
      status = ENOMEM;
      goto abort;
    }
    
    /* pin vpages in this pgd */
    pgd_vpages = MAL_MIN(vpages, MX_ATOVP(MX_MMT_PGD_SIZE 
					 - MX_MMT_PGD_OFFSET(va)));
    do {
      /* get the pmd and allocate it if needed */
      pmd = mx_mmt_get_pmd(es, pgd, va, 1);
      if (!pmd) {
	MX_WARN(("cannot allocate pmd\n"));
	status = ENOMEM;
	goto abort;
      }
      
      /* pin vpages in this pmd */
      pmd_vpages = MAL_MIN(pgd_vpages, MX_ATOVP(MX_MMT_PMD_SIZE 
					       - MX_MMT_PMD_OFFSET(va)));
      mal_assert(pmd_vpages <= MX_ADDRS_PER_VPAGE);
      
      if (memory_context & MX_PIN_FULLPAGES) {
#if 0
	/* FIXME */
	/* multiple fullpage segments */
	for (i = index ; i < pmd_vpages; i++, cur_va += MX_VPAGE_SIZE) {
	  pmd->pins[i].va = cur_useg->vaddr;
	  cur_useg->vaddr += MX_VPAGE_SIZE;
	  cur_useg->len -= MX_VPAGE_SIZE;
	  if (cur_useg->len == 0)
	    cur_useg += 1;
	}
#endif
      }
      
      i = MX_ATOVP(MX_MMT_PMD_OFFSET(va));
      status = mx_pin_vpages(es->is, &pmd->pins[i], &pmd->meta[i], pmd_vpages, 
			     flags, MX_PIN_CTX_TO_LOWLEVEL(memory_context));
      if (status) {
	MX_WARN(("cannot pin pages\n"));
	goto abort;
      }
      
      pgd_vpages -= pmd_vpages;
      vpages -= pmd_vpages;
      va += pmd_vpages * MX_VPAGE_SIZE;
    } while (pgd_vpages);
  } while (vpages);
  
  mx_mutex_exit(&es->sync);
  return 0;
  
 abort:
  mx_mutex_exit(&es->sync);
  mx_mmt_deregister(es, va_start, vpages_start, flags);
  return status;
}


int
mx_mmt_deregister(mx_endpt_state_t *es, uint64_t va, uint32_t vpages, uint32_t flags)
{
  uint32_t pgd_vpages, pmd_vpages, i;
  mx_mmt_pgd_t *pgd;
  mx_mmt_pmd_t *pmd;
  int status;
  
  mx_mutex_enter(&es->sync);
  do {
    /* lookup the pgd */
    pgd = mx_mmt_get_pgd(es, va, 0);
    if (!pgd) {
      MX_WARN(("cannot find pgd\n"));
      status = EINVAL;
      goto abort;
    }
    
    /* unpin vpages in this pgd */
    pgd_vpages = MAL_MIN(vpages, MX_ATOVP(MX_MMT_PGD_SIZE - MX_MMT_PGD_OFFSET(va)));
    do {
      /* lookup the pmd */
      pmd = mx_mmt_get_pmd(es, pgd, va, 0);
      if (!pmd) {
	MX_WARN(("cannot find pmd\n"));
	status = EINVAL;
	goto abort;
      }
      
      /* unpin vpages in this pmd */
      pmd_vpages = MAL_MIN(pgd_vpages, MX_ATOVP(MX_MMT_PMD_SIZE 
					       - MX_MMT_PMD_OFFSET(va)));
      mal_assert(pmd_vpages <= MX_ADDRS_PER_VPAGE);
      /* FIXME */
      for (i = 0; i < pmd_vpages; i++)
	pmd->meta[i].low = MX_DMA_INVALID_ENTRY;
      /* FIXME */
      i = MX_ATOVP(MX_MMT_PMD_OFFSET(va));
      mx_unpin_vpages(es->is, &pmd->pins[i], pmd_vpages, flags);
      
      pgd_vpages -= pmd_vpages;
      vpages -= pmd_vpages;
      va += pmd_vpages * MX_VPAGE_SIZE;
    } while (pgd_vpages);
  } while (vpages);

  mx_mutex_exit(&es->sync);
  return 0;
  
 abort:
  mx_mutex_exit(&es->sync);
  return status;
}

#endif

void
mx_mmu_init(mx_instance_state_t *is)
{
  int i, mcp_offset;

  /* init CRC32 table */
  mx_crc32_init(is);
  
  /* init host hash */
  bzero(is->mmu.host_hash, MX_MMU_HASH * sizeof(mx_mmu_t *));
  
  /* init host table */
  mcp_offset = MCP_GETVAL(is, mmu_table_offset);
  for (i = 0; i < MX_MMU_CNT; i++, mcp_offset += sizeof(mcp_mmu_t)) {
    is->mmu.host_table[i].next = &is->mmu.host_table[i+1];
    is->mmu.host_table[i].key = 0;
    is->mmu.host_table[i].mcp_offset = mcp_offset;
  }
  is->mmu.host_table[MX_MMU_CNT-1].next = 0;
  is->mmu.free = is->mmu.host_table;
}
