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

#include "mx_arch.h"
#include "mx_misc.h"
#include "mx_instance.h"
#include "mx_malloc.h"
#include "mx_pio.h"
#include "mal_stbar.h"
#include "myri_version.h"
#include "mal_cpu.h"

#include "kraw.h"
#include "myri_raw.h"


#include "mx_ether_common.h"

#ifndef MX_DISABLE_COMMON_COPYBLOCK
#define MX_DISABLE_COMMON_COPYBLOCK 0
#endif

#ifndef MX_DMA_VPAGE_SIZE 
#define MX_DMA_VPAGE_SIZE MX_VPAGE_SIZE
#define MX_ALLOC_DMA_PAGE(is, addr, pin, len) mx_alloc_dma_page(is, addr, pin)
#define MX_FREE_DMA_PAGE(is, addr, pin, len) mx_free_dma_page(is, addr, pin)
#endif

int mx_initialized = 0;
uint32_t myri_mx_small_message_threshold = 1024;
uint32_t myri_mx_medium_message_threshold = 4*1024;
uint32_t myri_security_disabled = 0;
mx_spinlock_t mx_lanai_print_spinlock;

mx_sync_t mx_global_mutex;

uint32_t myri_debug_mask;

uint32_t myri_msi_level = MAL_OS_LINUX && MAL_CPU_powerpc64;

mx_atomic_t myri_max_user_pinned_pages;

uint32_t myri_clksync_period = 1000; /* in millisecs */
uint32_t myri_clksync_error = 30000;
uint32_t myri_clksync_error_count = 0;
uint32_t myri_clksync_verbose = 0;

int
mx_init_driver(void)
{
#if MAL_CPU_x86 && MX_ENABLE_SSE2 && !defined _MSC_VER
  if (!mal__cpu_has_sse2()) {
    MX_WARN(("Processor without sse2: recompile with --disable-sse2.\n"));
    return EIO;
  }
#endif
  MX_INFO(("Version %s\n", MYRI_VERSION_STR));
  MX_INFO(("Build %s\n", MYRI_BUILD_STR));
  MX_INFO(("Debug %s\n", MAL_DEBUG ? "ON" : "OFF"));
  mx_set_default_hostname();

  mx_lx_init_board_ops();
  mx_lz_init_board_ops();

  MAL_DEBUG_PRINT 
    (MAL_DEBUG_BOARD_INIT,
     (MYRI_DRIVER_STR " configured for %d instances\n", myri_max_instance));
  mx_instances = mx_kmalloc 
    (sizeof(mx_instances[0]) * myri_max_instance, MX_MZERO);
  if (mx_instances == 0) {
    return ENOMEM;
  }


  mx_sync_init(&mx_global_mutex, NULL, -1, "mx_global_mutex");
  mx_spin_lock_init(&mx_lanai_print_spinlock, NULL, -1, "lanai print spinlock");
  mx_initialized = 1;

  if (myri_security_disabled) {
    MX_WARN(("Security is disabled\n"));
  }

  return 0;
}

int
mx_finalize_driver(void)
{
  if (!mx_initialized) {
    return -1;
  }
  mx_kfree(mx_instances);
  mx_instances = 0;
  mx_sync_destroy(&mx_global_mutex);
  mx_spin_lock_destroy(&mx_lanai_print_spinlock);
  return 0;
}

int
mx_alloc_zeroed_dma_page(mx_instance_state_t *is, char **addr, 
			 mx_page_pin_t *pin)
{
  int status;

  status = mx_alloc_dma_page(is, addr, pin);
  if (!status)
    bzero(*addr, PAGE_SIZE);
  return status;
}

int
mx_alloc_zeroed_dma_page_relax_order(mx_instance_state_t *is, char **addr, 
				     mx_page_pin_t *pin)
{
  int status;

  status = mx_alloc_dma_page_relax_order(is, addr, pin);
  if (!status)
    bzero(*addr, PAGE_SIZE);
  return status;
}

mx_instance_state_t *
mx_get_instance(uint32_t unit)
{
  mx_instance_state_t *is;

  /* make sure the requested unit is in range  */
  if (unit >= myri_max_instance) {
    return NULL;
  }

  /* grab the global mutex to make sure the global array of instances
     is consistant */
  mx_mutex_enter(&mx_global_mutex);
  is = mx_instances[unit];

  /* make sure its a valid instance */
  if (is == 0) {
    mx_mutex_exit(&mx_global_mutex);      
    return NULL;
  }
  
  mx_atomic_add(1, &is->ref_count);
  mx_mutex_exit(&mx_global_mutex);
  return is;
}

void
mx_release_instance(mx_instance_state_t *is)
{
  mx_atomic_subtract(1, &is->ref_count);
}

static int
mx_get_logging(mx_instance_state_t *is, uint32_t size, uaddr_t out,
	       uint32_t is_kernel)
{
  char *sram_ptr, *bounce;
  int status = 0;
  uint32_t logging_offset, dont_care, copy_size;

  /* sanity check */
  if (mx_is_dead(is)) {
    return (ENXIO);
  }
  
  /* sanity check */
  if (size == 0) {
    return (0);
  }
  
  /* get the SRAM offset of the logging data */
  logging_offset = MCP_GETVAL(is, logging_offset);
  
  /* sanity check */
  if (logging_offset == 0) {
    MX_WARN (("Logging is not enabled in the MCP build\n"));
    return (ENXIO);
  }
  
  /* the MCP is now logging */
  mx_mutex_enter(&is->sync);
  if (is->flags & MX_IS_LOGGING) {
    mx_mutex_exit(&is->sync);
    return EBUSY;
  }
  is->flags |= MX_IS_LOGGING;
  mx_mutex_exit(&is->sync);
  
  sram_ptr = (char *)is->lanai.sram + (size_t) logging_offset;
  bounce = mx_kmalloc(MX_VPAGE_SIZE, MX_WAITOK|MX_MZERO);
  if (bounce == 0) {
    status = ENOMEM;
    goto abort_with_sync;
  }

  /* tell the MCP to start logging */
  status = mx_mcp_command(is, MCP_CMD_START_LOGGING, size, 0, 0, &dont_care);
  if (status) {
    MX_WARN (("Couldn't start logging\n"));
    goto abort_with_buffer;
  }

  /* wait for the MCP to log the requested amount of information */
  status = mx_sleep(&is->logging.sync, MX_LOGGING_WAIT, MX_SLEEP_INTR);
  if (status) {
    if (status == EAGAIN)
      MX_WARN(("time out waiting for logging buffer\n"));
    mx_sync_reset(&is->logging.sync);
    goto abort_with_buffer;
  }
  
  /* retrieve the logging data */
  size = (size > is->logging.size) ? is->logging.size : size;
  while (size > 0) {
    copy_size = (size > MX_VPAGE_SIZE) ? MX_VPAGE_SIZE : size;
    mx_pio_bcopy_read(sram_ptr, bounce, copy_size);
    status = mx_copyout(bounce, out, copy_size, is_kernel);
    if (status) {
      MX_WARN(("failed to copy logging buffer out to 0x%lx\n", 
	       (unsigned long) out));
      break;
    }

    size -= copy_size;
    sram_ptr += copy_size;
    out += copy_size;
  }

 abort_with_buffer:
  mx_kfree(bounce);
   
 abort_with_sync:
  mx_mutex_enter(&is->sync);
  is->flags &= ~MX_IS_LOGGING;
  mx_mutex_exit(&is->sync);
  return status;
}

static int
mx_run_dmabench(mx_instance_state_t *is, mx_dmabench_t *x)
{
  mx_page_pin_t pin;
  char *addr;
  uint32_t dont_care;
  uint16_t count_size;
  int status, cmd;

  if ((x->count > 100) ||
      (1 << x->log_size) > MX_VPAGE_SIZE)
    return EINVAL;

  mx_mutex_enter(&is->sync);
  if (is->dmabench.busy) {
    mx_mutex_exit(&is->sync);
    return EBUSY;
  }
  is->dmabench.busy = 1;
  mx_mutex_exit(&is->sync);
  status = mx_alloc_dma_page_relax_order(is, &addr, &pin);
  if (status)
    goto abort_busy;

  if (x->dma_read)
    cmd = MCP_CMD_START_DMABENCH_READ;
  else 
    cmd = MCP_CMD_START_DMABENCH_WRITE;
  count_size = (x->count << 8) | x->log_size;
  status = mx_mcp_command(is, cmd, count_size, pin.dma.high,
			  pin.dma.low, &dont_care);

  if (status) {
    MX_WARN(("Could not start DMA benchmark, status = %d\n", status));
    MX_WARN(("0x%x 0x%x\n", pin.dma.high, pin.dma.low));
    goto abort_with_dmapage;
  }

  status = mx_sleep(&is->dmabench.wait_sync, MCP_COMMAND_TIMEOUT * 2,
		    MX_SLEEP_NOINTR);

  x->count = is->dmabench.count;
  x->cycles = is->dmabench.cycles;

 abort_with_dmapage:
  mx_free_dma_page(is, &addr, &pin);

 abort_busy:
  mx_mutex_enter(&is->sync);
  is->dmabench.busy = 0;
  mx_mutex_exit(&is->sync);
  return status;
}


#ifndef MX_FACTORIZED_PAGE_PIN 

/*
 * Take an array of pins and pin each underlying host page, and fill
 * in the DMA address for each vpage in the mdesc array.  Note that
 * PAGE_SIZE could be > MX_VPAGE_SIZE, in which case not all pins
 * would be used.
 */

int
mx_pin_vpages(mx_instance_state_t *is, mx_page_pin_t *pins, 
	      mcp_dma_addr_t *mdesc, int nvpages,
	      int flags, uint64_t memory_context)
{
  int i, status;
  uint64_t va;
  uint32_t page_offset;
  unsigned long pindex, prev_pindex;
  mx_page_pin_t *last_pin = NULL;


  mal_assert(nvpages <= MX_ADDRS_PER_VPAGE);
  va = pins[0].va;
  prev_pindex = MX_ATOP(va) - 1;
  for (i = 0; i < nvpages; i++) {
    page_offset = (uint32_t)(va - MX_PAGE_TRUNC(va));
    pindex = MX_ATOP(va);

    /* pin the underlying host page if needed */
    if (prev_pindex != pindex) {
      pins[i].va = MX_PAGE_TRUNC(va);
      status = mx_pin_page(is, &pins[i], flags, memory_context);
      if (status)
	goto abort_with_pins;
      last_pin = &pins[i];
    } else {
      pins[i].dma.low = MX_DMA_INVALID_ENTRY;
    }
    prev_pindex = pindex;

    /* save the dma address of the current page */
    if (i + 1 < nvpages)
      mdesc[i+1].low = MX_DMA_INVALID_ENTRY;
    MAL_STBAR();
    mdesc[i].high = htonl(last_pin->dma.high);
    mdesc[i].low = htonl(last_pin->dma.low + page_offset);

    va += (uint64_t)MX_VPAGE_SIZE;
  }

  return 0;

 abort_with_pins:
  mx_unpin_vpages(is, pins, i, flags);
  return status;
}

/*
 * Unpin all the host pages described in pins.  Note that when
 * PAGE_SIZE != MX_VPAGE_SIZE, some pin slots may not be valid.
 */

void
mx_unpin_vpages(mx_instance_state_t *is, mx_page_pin_t *pins, int npages, int flags)
{
  int i;

  for (i = 0; i < npages; i++)
    if (pins[i].dma.low != MX_DMA_INVALID_ENTRY)
      mx_unpin_page(is, &pins[i], flags);
}

#endif /* MX_FACTORIZED_PAGE_PIN */

static int
vpages_per_useg(mx_reg_seg_t *seg)
{
  uaddr_t start, end;
  int page_count;

  if (seg->len == 0)
    return 0;
  page_count = 0;
  start = (uaddr_t)seg->vaddr;
  end = start + seg->len - 1;
  page_count = (int)(MX_ATOVP(end) - MX_ATOVP(start) + 1);
 
  MAL_DEBUG_PRINT(0, ("%d pages for address 0x%lx, len = %d\n", 
		     page_count, (unsigned long) start, seg->len));
  mal_assert(page_count >= 1);
  return page_count;
}

static void
mx_free_dma_table (mx_instance_state_t *is, mx_dma_table_t *tbl)
{
  if (tbl->log_level > 0) {
    if (tbl->u.tables) {
      int i;
      for (i=0;i<tbl->nb_subtables;i++) {
	if (tbl->u.tables[i]) {
	  mx_free_dma_table(is, tbl->u.tables[i]);
	  mx_kfree(tbl->u.tables[i]);
	}
	tbl->u.tables[i] = 0;
      }
      mx_kfree(tbl->u.tables);
    tbl->u.tables = 0;
    }
  } else if (tbl->u.pins) {
    mx_kfree(tbl->u.pins);
    tbl->u.pins = 0;
  }
  if (tbl->desc) {
    mx_free_dma_page(is, (char**)&tbl->desc, &tbl->pin);
    tbl->desc = 0;
  }
  return;
}

static int
mx_fill_dma_table (mx_instance_state_t *is, mx_dma_table_t *tbl)
{
  int rc;
  long nb_entries = tbl->nb_entries;
  /* only case of nb_entries == 0 is rdma win of null length */
  mal_assert(nb_entries > 0 || tbl->log_level == 0);
  mal_assert(tbl->log_level >= 0);

  tbl->nb_subtables = 0;
  tbl->desc = 0;
  tbl->u.tables = 0;
  rc = mx_alloc_dma_page(is, (char**)&tbl->desc, &tbl->pin);
  mx_mem_check(!rc);
  tbl->desc[0].low = MX_DMA_INVALID_ENTRY;
  if (tbl->log_level == 0) {
    mal_assert(nb_entries <= MX_ADDRS_PER_VPAGE);
    if (nb_entries) {
      tbl->u.pins = mx_kmalloc(sizeof(*tbl->u.pins)*nb_entries, MX_WAITOK | MX_MZERO);
      mx_mem_check(tbl->u.pins);
    }
  } else {
    int i;
    long remain = nb_entries;
    long sub_max_size = 1 << tbl->log_level;
    /* alloc sub-tables */
    tbl->nb_subtables = MAL_ROUND_UP(nb_entries, sub_max_size) >> tbl->log_level;
    mal_assert(tbl->nb_subtables <= MX_ADDRS_PER_VPAGE);
    tbl->u.tables = mx_kmalloc(sizeof(tbl->u.tables[0])*tbl->nb_subtables, MX_WAITOK | MX_MZERO);
    mx_mem_check(tbl->u.tables);

    for (i=0;i<tbl->nb_subtables;i++) {
      int subentries;
      subentries = MIN(sub_max_size, remain);
      tbl->u.tables[i] = mx_kmalloc(sizeof(*tbl->u.tables[i]), MX_WAITOK | MX_MZERO);
      mx_mem_check(tbl->u.tables[i]);
      tbl->u.tables[i]->log_level = tbl->log_level - MX_ADDRS_PER_VPAGE_SHIFT;
      tbl->u.tables[i]->nb_entries = subentries;
      rc = mx_fill_dma_table(is, tbl->u.tables[i]);
      mx_mem_check(!rc);
      remain -= subentries;
      tbl->desc[i].high = htonl(tbl->u.tables[i]->pin.dma.high);
      tbl->desc[i].low = htonl(tbl->u.tables[i]->pin.dma.low);
    }
    if (tbl->nb_subtables <MX_ADDRS_PER_VPAGE) {
      tbl->desc[tbl->nb_subtables].low = MX_DMA_INVALID_ENTRY;
    }
  }
  return 0;

 handle_enomem:
  mx_free_dma_table(is,tbl);
  return ENOMEM;
}


static struct mx_dma_table *
mx_dma_table_for_seg(mx_host_dma_win_t *hc, long hseg)
{
  struct mx_dma_table *tbl;
  tbl = &hc->table;
  mal_assert(hseg < (MX_ADDRS_PER_VPAGE << tbl->log_level));
  mal_assert(hseg < tbl->nb_entries);
  while (tbl->log_level) {
    int tbl_index = (hseg >> tbl->log_level) & (MX_ADDRS_PER_VPAGE - 1);
    mal_assert(tbl_index < tbl->nb_subtables);
    tbl = tbl->u.tables[tbl_index];
  }
  mal_assert((hseg & (MX_ADDRS_PER_VPAGE - 1)) < tbl->nb_entries);
  return tbl;
}

void
mx_free_dma_win (mx_instance_state_t *is, mx_host_dma_win_t *hc)
{
  mx_free_dma_table(is, &hc->table);
  mx_kfree(hc);
}


/* Allocate a new host dma chunk container, and its associated vpages 
   The host dma chunk structure consists of an array of page pin
   info for all the pages in the send or receive, plus a linked list
   of all the mcp dma chunk structures.

   The mcp dma chunk structures contain a pointer to a vpage, where
   DMA addresses are stored in network byte order, and which the MCP 
   reads.  It also contains the pin for that vpage, and a pointer
   to the next chunk in the list.
*/

mx_host_dma_win_t *
mx_new_dma_win(mx_instance_state_t *is, int nsegs)
{
  mx_host_dma_win_t *hc;
  int rc;

  hc = mx_kmalloc(sizeof(*hc), MX_WAITOK | MX_MZERO);
  mx_mem_check(hc);

  hc->table.nb_entries = nsegs;
  while ((MX_ADDRS_PER_VPAGE << hc->table.log_level) < nsegs) {
    hc->table.log_level += MX_ADDRS_PER_VPAGE_SHIFT;
  }
  rc = mx_fill_dma_table(is,&hc->table);
  mx_mem_check(!rc);

  return hc;

 handle_enomem:
  if (hc != NULL)
    mx_free_dma_win(is, hc);
  return NULL;
}

/* write a rdma window to the mcp */
static void
mx_setup_mcp_rdma_window(mx_endpt_state_t *es, int handle)
{
  mx_host_dma_win_t *hc;
  mcp_rdma_win_t *mcp_window;

  hc = es->host_rdma_windows[handle].win;
  mal_always_assert(hc);

  /* make sure the host part of the window is uptodate before giving it to the NIC */
  MAL_STBAR();

  mcp_window = &(es->mcp_rdma_windows[handle]);
  MX_PIO_WRITE(&(mcp_window->length), htonl(hc->length));
  MX_PIO_WRITE(&(mcp_window->log_level), htonl(hc->table.log_level));
  MX_PIO_WRITE(&(mcp_window->table.high), htonl(hc->table.pin.dma.high));
  MX_PIO_WRITE(&(mcp_window->table.low), htonl(hc->table.pin.dma.low));
  /* write the seqnum so that the window is really valid */
  MX_PIO_WRITE(&(mcp_window->seqnum), htonl(hc->seqnum));
}

static int
mx_register(mx_endpt_state_t *es, uint32_t handle, uint32_t seqnum,
	    uint32_t num_usegs, mx_reg_seg_t *usegs,
	    uintptr_t memory_context)
{
  uint32_t flags;
  mx_instance_state_t *is = es->is;
  mx_host_dma_win_t *hc;
  int hseg, status, num_hsegs, npages;
  uint32_t length;
  struct mx_dma_table *tbl;
  mx_reg_seg_t * cur_useg;
  uint64_t cur_va;
  int i;

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
    num_hsegs = 0;
    length = 0;
    for(i=0; i<num_usegs; i++) {
      if (MX_VPAGE_OFFSET(usegs[i].vaddr) || (usegs[i].len & (MX_VPAGE_SIZE - 1))) {
	MX_WARN(("mx_register: fullpage segments must be entire single pages\n"));
	return EINVAL;
      }
      length += usegs[i].len;
      num_hsegs += usegs[i].len / MX_VPAGE_SIZE;
    }
  } else {
    /* only one segment is supported in the general case */
    if (num_usegs > 1) {
      MX_WARN(("mx_register: scatter/gather is not yet supported \n"));
      return EINVAL;
    }

    /* use the first segment */
    length = usegs[0].len;
    if (MX_VPAGE_OFFSET(usegs[0].vaddr) || MX_VPAGE_OFFSET(length)) {
      MX_WARN(("mx_register: rdma window must use page boundaries\n"));
      return EINVAL;
    }

    /* find the number of dma segments required for this registration */
    num_hsegs = vpages_per_useg(&usegs[0]);
  }

  if (num_hsegs < 0 || num_hsegs >= 0x7FFFFFFF / MX_PAGE_SIZE) {
    return EINVAL;
  }

  /* grab the es mutex, so as to ensure that no other thread is racing
     to register something on this handle.  This is theoretically not
     needed, but is here to protect us from malicous users */
  mx_mutex_enter(&es->sync);

  if (es->host_rdma_windows[handle].win) {
    /* update the seqnum */
    es->host_rdma_windows[handle].win->seqnum = seqnum;

    /* reinitialize the handle for the MCP and leave, since the lib
     * is supposed to reuse the same window for the same addresses
     */
    mx_setup_mcp_rdma_window(es, handle);

    status = 0;
    goto abort;
  }

  mal_assert(MX_PIO_READ(&(es->mcp_rdma_windows[handle].table.low)) == MX_DMA_INVALID_ENTRY);

  /* attempt to allocate a container for them, ensure we have at least
     one descriptor to avoid special cases */
  hc =  mx_new_dma_win(is, num_hsegs);
  if (!hc) {
    MX_WARN(("cannot mx_new_dma_win(%d)\n", num_hsegs));
    status = ENOMEM;
    goto abort;
  }

  /* walk the region, pinning pages as we go and storing the DMA address
     in the mcp chunks */

  if (length == 0) {
    /* make first dma desc points to the directory page, the mcp might
       use it with a zero-length dma */
    mal_assert(hc->table.log_level == 0);
    hc->table.desc[0].low = MX_DMA_INVALID_ENTRY;
  }

  flags = MX_PIN_STREAMING | MX_PIN_CTX_TO_TYPE(memory_context);
  hc->flags = flags;
  hc->length = length;
  hc->seqnum = seqnum;

  cur_useg = &usegs[0];
  cur_va = cur_useg->vaddr;
  for (hseg = 0; hseg < num_hsegs; hseg += MX_ADDRS_PER_VPAGE) {
    npages = MIN(num_hsegs - hseg, MX_ADDRS_PER_VPAGE);
    tbl = mx_dma_table_for_seg(hc, hseg);
    mal_assert(npages <= tbl->nb_entries);

    if (memory_context & MX_PIN_FULLPAGES) {
      /* multiple fullpage segments */
      for(i = 0; i < npages; i++) {
	tbl->u.pins[i].va = cur_useg->vaddr;
	cur_useg->vaddr += MX_VPAGE_SIZE;
	cur_useg->len -= MX_VPAGE_SIZE;
	if (cur_useg->len == 0)
	  cur_useg += 1;
      }
    } else {
      /* general case, with only one segment */
      for(i = 0; i < npages; i++, cur_va += MX_VPAGE_SIZE)
        tbl->u.pins[i].va = cur_va;
    }

    status = mx_pin_vpages(is, tbl->u.pins, tbl->desc, npages, 
			   flags, MX_PIN_CTX_TO_LOWLEVEL(memory_context));
    if (status) {
      MX_WARN(("cannot pin pages\n"));
      for(i = 0; i < npages; i++)
	tbl->u.pins[i].va = 0;
      goto abort_with_pins;
    }
  }

  /* XXX add some code here to tell the mcp where to start */

  if (es->host_rdma_windows[handle].win) {
    status = EBUSY;
    MX_WARN(("mx_register: lost race to register handle %d\n", handle));
    goto abort_with_pins;
  }
  es->host_rdma_windows[handle].win = hc;
  mx_setup_mcp_rdma_window(es, handle);

  mx_mutex_exit(&es->sync);

  return 0;

 abort_with_pins:
  for (hseg = 0; hseg < num_hsegs; hseg += MX_ADDRS_PER_VPAGE) {
    tbl = mx_dma_table_for_seg(hc,hseg);
    npages = MIN(num_hsegs - hseg, MX_ADDRS_PER_VPAGE);
    mal_assert(npages <= tbl->nb_entries);
    if (tbl->u.pins[0].va)
      mx_unpin_vpages(is, tbl->u.pins, npages, flags);
  }
  mx_free_dma_win(is, hc);
  es->host_rdma_windows[handle].win = NULL;

 abort:
  mx_mutex_exit(&es->sync);
  return status;
}

static int
mx_deregister(mx_endpt_state_t *es, uint32_t handle)
{
  int hseg, npages;
  mx_instance_state_t *is = es->is;
  mx_host_dma_win_t *hc;
  int flags;

  mx_mutex_enter(&es->sync);
  if (es->host_rdma_windows[handle].win == 0) {
    mx_mutex_exit(&es->sync);
    return EINVAL;
  }
  hc = es->host_rdma_windows[handle].win;
  es->host_rdma_windows[handle].win = 0;
#if 0
  /* might warn in case of parity */
  mal_assert(MX_PIO_READ(&(es->mcp_rdma_windows[handle].addr.low)) != MX_DMA_INVALID_ENTRY);
#endif
  /* FIXME: we trust the lib that the MCP has finished using this
   window, and mark it done free ourselves eventually the MCP should
   mark it unused when it's done and we should check that it is before
   freeing anything */
  es->mcp_rdma_windows[handle].table.low = MX_DMA_INVALID_ENTRY;
  mx_mutex_exit(&es->sync);

  flags = hc->flags;

  for (hseg = 0; hseg < hc->table.nb_entries; hseg += MX_ADDRS_PER_VPAGE) {
    struct mx_dma_table *tbl = mx_dma_table_for_seg(hc, hseg);
    npages = MIN(hc->table.nb_entries - hseg, MX_ADDRS_PER_VPAGE);
    mal_assert(npages <= tbl->nb_entries);
    mx_unpin_vpages(is, tbl->u.pins, npages, flags);
  } 	

  mx_free_dma_win(es->is, hc);
  return 0;
}


/*
 * Iterate over a locked endpoint state, calling testfp()
 * on each pin held by this endpoint.  This is needed for
 * Solaris to be able to release pinned memory when exiting
 * uncleanly 
 */

mx_page_pin_t *
mx_find_pin(mx_endpt_state_t *es,  int (*testfp)(mx_page_pin_t *pin, void *arg),
	    void *arg)
{
  int hseg, npages, i, handle, found;
  mx_host_dma_win_t *hc;

  for (handle = 0; handle < myri_mx_max_rdma_windows; handle++) {
    if (es->host_rdma_windows == NULL)
      return NULL;

    if (es->host_rdma_windows[handle].win == 0)
      continue;

    hc = es->host_rdma_windows[handle].win;
    for (hseg = 0; hseg < hc->table.nb_entries; hseg += MX_ADDRS_PER_VPAGE) {
      struct mx_dma_table *tbl = mx_dma_table_for_seg(hc, hseg);
      npages = MIN(hc->table.nb_entries - hseg, MX_ADDRS_PER_VPAGE);
      mal_assert(npages <= tbl->nb_entries);
      for (i = 0; i < npages; i++) {
	found = (*testfp)(&tbl->u.pins[i], arg);
	if (found)
	  return &tbl->u.pins[i];
      }
    } 	
  }
  return NULL;
}

static void
mx_recover_rdma_windows(mx_endpt_state_t *es)
{
  int handle;
  for (handle = 0; handle < myri_mx_max_rdma_windows; handle++) {
    if (!es->host_rdma_windows[handle].win)
      continue;
    mx_setup_mcp_rdma_window(es, handle);
  }
}

#if !MX_DISABLE_COMMON_COPYBLOCK

#if MAL_OS_WINDOWS
void mx_win_alloc_copyblock(mx_copyblock_t *cb);
void mx_win_free_copyblock(mx_copyblock_t *cb);
#endif

void
mx_common_free_copyblock(mx_instance_state_t *is, mx_copyblock_t *cb)
{
  unsigned i;

  if (cb->pins != NULL) {
    for (i = 0; i < (cb->size / PAGE_SIZE); i++) {
      if (cb->pins[i].va != 0) {
	mx_unpin_page(is, &cb->pins[i], MX_PIN_KERNEL | MX_PIN_CONSISTENT);
	mx_unreserve_page((void *)(uintptr_t)cb->pins[i].va);
      }
    }
    mx_kfree(cb->pins);
  }
  if (cb->alloc_addr != NULL) {
#if MAL_OS_WINDOWS
    mx_win_free_copyblock(cb);
#else
    mx_kfree(cb->alloc_addr);
#endif
  }
  if (cb->dmas != NULL)
    mx_kfree(cb->dmas);

  bzero(cb, sizeof(*cb));
}

int
mx_common_alloc_copyblock(mx_instance_state_t *is, mx_copyblock_t *cb)
{
  int i, status;
  uint64_t va;
  uint32_t len;

  len = cb->size;

  /* make sure that the length is page aligned */
  mal_always_assert((len & (PAGE_SIZE - 1)) == 0);

#if MAL_OS_WINDOWS
  mx_win_alloc_copyblock(cb);
#else
  cb->alloc_addr = mx_kmalloc(len + PAGE_SIZE, MX_MZERO|MX_WAITOK);
#endif
  cb->pins = mx_kmalloc(sizeof(cb->pins[0]) * (len / PAGE_SIZE), MX_MZERO|MX_WAITOK);
  cb->dmas = mx_kmalloc(sizeof(mcp_dma_addr_t) * (len / MX_VPAGE_SIZE), MX_MZERO|MX_WAITOK);
  if ((cb->alloc_addr == NULL) || (cb->pins == NULL) || (cb->dmas == NULL)) {
    MX_WARN(("copyblock pin info allocation failed due to lack of memory\n"));
    status = ENOMEM;
    goto abort_with_cb;
  }
  cb->addr =  (void *)(uintptr_t)MX_PAGE_ALIGN((uintptr_t)cb->alloc_addr);
  
  for (i = 0, va = (uint64_t)(size_t)cb->addr; 
       va < (uint64_t)(size_t)((char *)cb->addr + len); 
       va += PAGE_SIZE, i++) {
    cb->pins[i].va = va;
    status = mx_pin_page(is, &cb->pins[i], MX_PIN_KERNEL|MX_PIN_CONSISTENT, 0);
    if (status) {
      MX_WARN(("Failed to pin copyblock, status = %d\n", status));	
      cb->pins[i].va = 0;
      goto abort_with_cb;
    }
    mx_reserve_page((void *)(uintptr_t)va);
  }
  
  /* fill up the dma ring */
  for (i = 0; i < ((len + (MX_VPAGE_SIZE - 1)) / MX_VPAGE_SIZE); i++) {
    cb->dmas[i].high = htonl(cb->pins[i/(PAGE_SIZE/MX_VPAGE_SIZE)].dma.high);
    cb->dmas[i].low = htonl(cb->pins[i/(PAGE_SIZE/MX_VPAGE_SIZE)].dma.low);
  }
       
  return(0);

 abort_with_cb:
  mx_common_free_copyblock(is, cb);
  return (status);
}
#endif

void
mx_free_huge_copyblock(mx_instance_state_t *is, mx_huge_copyblock_t *hcb)
{
  char *c;
  unsigned long i;
  unsigned long npages = (hcb->size + PAGE_SIZE - 1) / PAGE_SIZE;

  if (hcb->kva != NULL)
    mx_unmap_huge_copyblock(hcb);

  if (hcb->pins != NULL) {
    for (i = 0; i < npages; i++) {
      if (hcb->pins[i].va != 0) {
	c = (char *)(uintptr_t)hcb->pins[i].va;
	mx_unreserve_page(c);
	mx_free_dma_page(is, &c, &hcb->pins[i]);
      }
    }
    mx_kfree(hcb->pins);
  }
  hcb->pins = NULL;
  if (hcb->cb.pins != NULL)
    mx_free_copyblock(is, &hcb->cb);
  hcb->cb.pins = NULL;
  hcb->size = 0;
  mx_atomic_add(hcb->pinned_pages, &myri_max_user_pinned_pages);
  hcb->pinned_pages = 0;
}


int
mx_alloc_huge_copyblock(mx_instance_state_t *is, 
			mx_huge_copyblock_t *hcb,
			unsigned long lib_offset,
                        int node_id)
{
  mcp_dma_addr_t *dma;
  char *c;
  unsigned long i, addr, host_index, host_offset, pin_offset, pin_index;
  unsigned long npages = (hcb->size + PAGE_SIZE - 1) / PAGE_SIZE;
  unsigned long nvpages =  (hcb->size - lib_offset + MX_VPAGE_SIZE - 1) / MX_VPAGE_SIZE;
  int status;

  hcb->pinned_pages = 0;
  if (mx_atomic_read(&myri_max_user_pinned_pages) < npages) {
    hcb->size = 0;
    status = ENOMEM;
    goto abort;
  }
  mx_atomic_subtract(npages, &myri_max_user_pinned_pages);
  hcb->pinned_pages = npages;

  /* do the hard part first, and allocate big gobs of RAM */
  hcb->pins = mx_kmalloc_node(sizeof(hcb->pins[0]) * npages, MX_MZERO|MX_WAITOK, node_id);
  if ((hcb->pins == NULL)) {
    MX_WARN(("huge copyblock allocation failed due to lack of memory\n"));
    status = ENOMEM;
    goto abort;
  }

  for (i = 0; i < npages; i++) {
    status = mx_alloc_zeroed_dma_page(is, &c, &hcb->pins[i]);
    if (status) {
      MX_WARN(("Failed to alloc copyblock page %ld/%ld, status = %d\n",
	       i, npages, status));
      hcb->pins[i].va = 0;
      goto abort;
    }
    mx_reserve_page(c);
  }

  /* Allocate some pinnable memory to hand to the MCP, just use
     existing copyblock routines for ease
   */
  hcb->cb.size = nvpages * sizeof(mcp_dma_addr_t);
  status = mx_alloc_copyblock(is, &hcb->cb);
  if (status) {
    MX_WARN(("mx_alloc_huge_copyblock:copyblock alloc failure\n"));
    goto abort;
  }

  /* fill the copyblock with the the vpage dma addresses, we start
     lib_offset pages into the copyblock, so as to allow the lib
     to map some blank pages in front of the data copyblock
  */
  pin_index = 0;
  pin_offset = 0;
  for (addr = lib_offset; addr < hcb->size; addr += MX_VPAGE_SIZE) {
    dma = (mcp_dma_addr_t *)(unsigned long)(hcb->cb.pins[pin_index].va + pin_offset);
    host_index = MX_ATOP(addr);
    host_offset = (uint32_t)(addr - MX_PAGE_TRUNC(addr));
#if 0
    printk("Filling: addr: %ld host_index, %ld host_offset: %ld,  pin_index: %ld,  pin_offset: %ld\n",
	   addr, host_index, host_offset, pin_index, pin_offset);
#else
    dma->high = htonl(hcb->pins[host_index].dma.high);
    dma->low = htonl(hcb->pins[host_index].dma.low + host_offset);
#endif
    pin_offset += sizeof(mcp_dma_addr_t);
    if (pin_offset >= MX_PAGE_SIZE) {
      pin_offset = 0;
      pin_index++;
    }
  }

  return 0;

 abort:
  mx_free_huge_copyblock(is, hcb);
  return status;
}

#if MX_KERNEL_LIB
#define mx_endpt_alloc_copyblock(es,cb)					\
  ((es)->is_kernel ? mx_common_alloc_copyblock((es)->is,cb)		\
   : mx_alloc_copyblock((es)->is,cb))
#define mx_endpt_alloc_copyblock_relax_order(es,cb)			\
  ((es)->is_kernel ? mx_common_alloc_copyblock((es)->is,cb)		\
   : mx_alloc_copyblock_relax_order((es)->is,cb))
#define mx_endpt_free_copyblock(es,cb)			   \
  ((es)->is_kernel ? mx_common_free_copyblock((es)->is,cb) \
   : mx_free_copyblock((es)->is,cb))
#define mx_endpt_alloc_huge_copyblock(es,cb)				\
   mx_alloc_huge_copyblock((es)->is,cb,0,-1)
#define mx_endpt_alloc_huge_copyblock_relax_order(es,cb)		\
   mx_alloc_huge_copyblock_relax_order((es)->is,cb,0)
#define mx_endpt_free_huge_copyblock(es,cb)			\
   mx_free_huge_copyblock((es)->is,cb)
#else
#define mx_endpt_alloc_copyblock(es,cb) mx_alloc_copyblock((es)->is,cb)
#define mx_endpt_alloc_copyblock_relax_order(es,cb) mx_alloc_copyblock_relax_order((es)->is,cb)
#define mx_endpt_free_copyblock(es,cb) mx_free_copyblock((es)->is,cb)
#if MAL_OS_WINDOWS
#define mx_endpt_alloc_huge_copyblock(es,cb) mx_win_alloc_huge_copyblock((es)->is,cb,0)
#define mx_endpt_alloc_huge_copyblock_relax_order(es,cb) mx_win_alloc_huge_copyblock_relax_order((es)->is,cb,0)
#define mx_endpt_free_huge_copyblock(es,cb) mx_win_free_huge_copyblock((es)->is,cb)
#else
#define mx_endpt_alloc_huge_copyblock(es,cb) mx_alloc_huge_copyblock((es)->is,cb,0,-1)
#define mx_endpt_alloc_huge_copyblock_relax_order(es,cb) mx_alloc_huge_copyblock_relax_order((es)->is,cb,0)
#define mx_endpt_free_huge_copyblock(es,cb) mx_free_huge_copyblock((es)->is,cb)
#endif
#endif

static int
mx_alloc_copyblocks(mx_endpt_state_t *es)
{
  int status;
  uint32_t offset;
  mx_instance_state_t *is;

  is = es->is;

  es->sendq.size = (uint32_t)MX_VPTOA(MCP_SENDQ_VPAGE_CNT);
  status = mx_endpt_alloc_copyblock_relax_order(es, &es->sendq);
  if (status)
    goto abort_with_nothing;

  es->recvq.size = (uint32_t)MX_VPTOA(myri_recvq_vpage_cnt);
  /* FIXME: need relax_order variant of huge copyblock */
  /* status = mx_endpt_alloc_huge_copyblock_relax_order(es, &es->recvq); */
  status = mx_endpt_alloc_huge_copyblock(es, &es->recvq);
  if (status)
    goto abort_with_sendq;

  es->eventq.size = (uint32_t)MX_VPTOA(myri_eventq_vpage_cnt);
  status = mx_endpt_alloc_copyblock(es, &es->eventq);
  if (status)
    goto abort_with_recvq;

  /* this "copyblock" is really NIC SRAM; don't allocate anything.
     Just figure out where it lives.  We have the entire nic mapped
     into SRAM, so there's nothing to undo if anything fails. 
  */
  es->user_mmapped_sram.size = MX_PAGE_ALIGN(MCP_UMMAP_SIZE);
  status = mx_mcp_command(is, MCP_CMD_GET_USER_MMAP_OFFSET,
			  es->endpt, 0, 0, &offset);
  if (status) {
    MX_WARN (("Can't determine user mmap offset \n"));
    goto abort_with_eventq;
  }

  mal_always_assert((MX_PAGE_SIZE <= MCP_UMMAP_SIZE && 
		     offset % MX_PAGE_SIZE == 0) ||
		    (MX_PAGE_SIZE > MCP_UMMAP_SIZE &&
		     offset % MX_PAGE_SIZE ==
		     (es->endpt * MCP_UMMAP_SIZE) % MX_PAGE_SIZE));
  /* with big page size (> UMMAP_SIZE), we have to map the whole page
     (i.e. we also map some siblings endpoints */
  offset &= ~(MX_PAGE_SIZE - 1);

  es->user_mmapped_sram.addr = 
    ((char *)(is->lanai.sram + (unsigned long)offset));

  if (offset + es->user_mmapped_sram.size > is->sram_size) {
    MX_WARN (("User mapped sram has bad size (0x%x) or location (0x%x)\n",
	      es->user_mmapped_sram.size, offset));
  }
  es->user_mmapped_zreq.addr = (char*)is->lanai.sram + (1<<23) + (es->endpt << 16);
  if (MAL_IS_ZE_BOARD(is->board_type)) {
    /* we have only 4MB of virtual space */
    if (es->endpt >= (0x400000 >> 16)) {
      MX_WARN (("Endpoint number %d too big\n", es->endpt));
      status = EINVAL;
      goto abort_with_eventq;
    }
    es->user_mmapped_zreq.size = MX_PAGE_ALIGN(MCP_UMMAP_SIZE);
  }
  return (0);

 abort_with_eventq:
  mx_endpt_free_copyblock(es, &es->eventq);
  
 abort_with_recvq:
  mx_endpt_free_huge_copyblock(es, &es->recvq);
  
 abort_with_sendq:
  mx_endpt_free_copyblock(es, &es->sendq);

  /* make sure mx_common_close won't try a second time */
  es->eventq.pins = es->recvq.pins = es->sendq.pins = 0;

 abort_with_nothing:
  return (status);
}


int
mx_dma_map_copyblock(mx_endpt_state_t *es, mx_copyblock_t *cb, uint32_t offset)
{
  mx_instance_state_t *is;
  mcp_dma_addr_t *dma;
  uint32_t host_offset;
  uaddr_t mcp_index, host_index;
  unsigned long addr;

  is = es->is;
  dma = (mcp_dma_addr_t *)((char *)(is->lanai.sram + (unsigned long)offset));

  /* Walk the copyblock, storing the DMA address for each virtual page
     down on the nic.  Note that on hosts with large page sizes, two
     or more virtual pages may map to offsets within the same host
     page. */
     
  for (addr = 0; addr < cb->size; addr += MX_VPAGE_SIZE) {
    mcp_index = MX_ATOVP(addr);
    host_index = MX_ATOP(addr);

    /* compute the offset into the (possibly larger) host page */
    host_offset = (uint32_t)(addr - MX_PAGE_TRUNC(addr));
    
#if 1   /* assume 64 bit-writes are not causing padding fifo overflow */
    MX_PIO_WRITE((uint64_t*)&dma[mcp_index], 
		 mx_hton_u64(((uint64_t)cb->pins[host_index].dma.high << 32) +
			     cb->pins[host_index].dma.low + host_offset));
#else
    MX_PIO_WRITE(&(dma[mcp_index].high), htonl(cb->pins[host_index].dma.high));
    MX_PIO_WRITE(&(dma[mcp_index].low), htonl(cb->pins[host_index].dma.low + host_offset));
#endif
    MAL_STBAR();
    
  }
  return (0);
}

static int
mx_dma_map_copyblocks(mx_endpt_state_t *es)
{
  uint32_t offset;
  int status = 0;
  
  status = mx_mcp_command(es->is, MCP_CMD_GET_ENDPT_MAP_OFFSET, 
			  es->endpt, 0, 0, &offset);
  if (status) {
    MX_WARN (("could not find get endpoint map offset\n"));
    return (status);
  }
  
  status |= mx_dma_map_copyblock(es, &es->sendq, offset);
  offset += MCP_SENDQ_VPAGE_CNT * sizeof(mcp_dma_addr_t);
  status |= mx_dma_map_copyblock(es, &es->recvq.cb, offset);
  offset += es->recvq.cb.size / (MX_VPAGE_SIZE / sizeof(mcp_dma_addr_t));
  status |= mx_dma_map_copyblock(es, &es->eventq, offset);

  return (status);
}



/* basic opening of an endpoint from scratch, still needs to be enabled afterwards */
static int
mx_open_mcp_endpoint(mx_endpt_state_t *es)
{
  mx_instance_state_t *is = es->is;
  uint32_t dummy;
  int status;

  /* open the endpoint on the mcp */
  status = mx_mcp_command(is, MCP_CMD_OPEN_ENDPOINT, es->endpt, 0, 0, &dummy);
  if (status != 0) {
    MX_WARN(("%s: endpt %d, mcp open failed\n", is->is_name, es->endpt));
    return (status);
  }

  return 0;
}

/* either finish the opening or an endpoint, or re-enable it after recovery */
static int
mx_enable_mcp_endpoint(mx_endpt_state_t *es)
{
  mx_instance_state_t *is = es->is;
  uint32_t dummy;
  int status;

  
  /* tell the MCP about the copyblock's DMA addresses */
  status = mx_dma_map_copyblocks(es);
  if (status) {
    MX_WARN(("%s: endpt %d, mapping copyblock failed\n", is->is_name, es->endpt));
    return (status);
  }

  /* set the endpoint session id */
  status = mx_mcp_command(is, MCP_CMD_SET_ENDPOINT_SESSION, es->endpt,
			  es->session_id, 0, &dummy);
  if (status != 0) {
    MX_WARN(("%s: endpt %d, set endpt session_id failed\n", is->is_name, es->endpt));
    return (status);
  }

  /* restore existing rdma windows (parity error recovery) */
  mx_recover_rdma_windows(es);
 
  /* enable the endpoint on the mcp */
  status = mx_mcp_command(is, MCP_CMD_ENABLE_ENDPOINT, es->endpt, 0, 0, &dummy);
  if (status != 0) {
    MX_WARN(("%s: endpt %d, endpt enable failed\n", is->is_name, es->endpt));
    return (status);
  }

  return 0;
}

int
mx_common_open(int32_t unit, int32_t endpoint, mx_endpt_state_t *es, 
               enum myri_endpt_type es_type)
{
  int status = 0;
  mx_instance_state_t *is;

  is = mx_get_instance(unit);
  if (is == 0)
    return (ENODEV);

  mx_mutex_enter(&is->sync);

  if (mx_is_dead(is)) {
    mx_mutex_exit(&is->sync);
    mx_release_instance(is);
    return (ENXIO);
  }

  /* make sure that the requested endpoint is in range */
  if (es_type == MYRI_ES_MX && (endpoint < 0 || endpoint >= is->es_count)) {
    mx_mutex_exit(&is->sync);
    mx_release_instance(is);
    return (ENODEV);
  }

  if (es_type == MYRI_ES_MX) {
    int busy, i, ep_per_page;
    unsigned ep_block_start;
    busy = 0;
    if (is->es[endpoint] != 0)
      busy = 1;
    ep_per_page = MX_PAGE_SIZE / MCP_UMMAP_SIZE;
    if (ep_per_page > 1) {
      ep_block_start = endpoint & ~(ep_per_page - 1);
      for (i = ep_block_start; i < ep_block_start + ep_per_page; i++) {
	if (is->es[i] && is->es[i]->euid != es->euid) {
	  busy = 1;
	  break;
	}
      }
    }
    if (busy) {
      mx_mutex_exit(&is->sync);
      mx_release_instance(is);
      return (EBUSY);
    }

    is->es[endpoint] = es;
    es->is = is;
    es->endpt = endpoint;
    es->session_id = mx_rand() | 0x40000000U;
    es->session_id &= ~0x80000000U;
  } else if (es_type == MYRI_ES_RAW) {
    /* can have just one open of raw message api */
    if (is->raw.use_count != 0) {
      mx_mutex_exit(&is->sync);
      mx_release_instance(is);
      return (EBUSY);
    }
    /* and the endpoint must be privileged */
    if ((es->privileged | myri_security_disabled) == 0) {
      mx_mutex_exit(&is->sync);
      mx_release_instance(is);
      return EPERM;
    }
    es->is = is;
    is->raw.use_count++;
    mx_mutex_exit(&is->sync);
    status = myri_kraw_init(is);
    mx_mutex_enter(&is->sync);
    is->raw.es = es;
    if (status) {
      MX_WARN(("%s: failed to setup raw interface. Status %d\n", 
	       is->is_name, status));
      is->raw.es = 0;
      es->is = 0;
      is->raw.use_count--;
      mx_mutex_exit(&is->sync);
      mx_release_instance(is);
      return status;
    }
  }
  else if (es_type == MYRI_ES_SNF_RX) {
    int min_endpt, max_endpt, busy, i;
    busy = 0;
    min_endpt = max_endpt = 0;
    if (endpoint == SNF_ENDPOINT_RX) {
      min_endpt = myri_max_endpoints;
      max_endpt = min_endpt + myri_max_endpoints - 1;
    }
    else if (endpoint == SNF_ENDPOINT_RX_RING) {
      min_endpt = myri_max_endpoints * 2;
      max_endpt = min_endpt + myri_max_endpoints - 1;
    }
    else if (endpoint == SNF_ENDPOINT_RX_BH) {
      min_endpt = max_endpt = myri_max_endpoints * 3;
      if (!es->privileged) {
        mx_mutex_exit(&is->sync);
        mx_release_instance(is);
        return EPERM;
      }
    }
    else {
      MX_WARN(("invalid endpoint number %d\n", endpoint));
      mx_mutex_exit(&is->sync);
      mx_release_instance(is);
      return EINVAL;
    }

    busy = 1;

    for (i = min_endpt; i <= max_endpt; i++) {
      if (is->es[i] == NULL) {
        busy = 0;
        break;
      }
    }
    if (busy) {
      mx_mutex_exit(&is->sync);
      mx_release_instance(is);
      return EBUSY;
    }
    if (endpoint == SNF_ENDPOINT_RX_BH)
      es->snf.ring_id = SNF_RING_KAGENT;
    else
      es->snf.ring_id = SNF_RING_UNUSED;
    is->es[i] = es;
    es->endpt = i;
    es->is = is;
  }
  else
    return EINVAL;
  es->ref_count = 0;
  mx_sync_init(&es->sync, is, endpoint, "es->sync");
  mx_sync_init(&es->wait_sync, is, endpoint, "es->wait_sync");
  mx_sync_init(&es->app_wait_sync, is, endpoint, "es->app_wait_sync");
  mx_sync_init(&es->close_sync, is, endpoint, "es->close_sync");
  es->snf.flags = 0;
  es->snf.tx_intr_requested = 0;
  mx_spin_lock_init(&es->snf.tx_lock, is, es->endpt, "es->snf.tx_lock");

  es->cpu_id = -1;
  es->node_id = -1;
  es->es_type = es_type;
  mx_mutex_exit(&is->sync);

  if (es_type != MYRI_ES_MX)
    return 0;

  /* allocate the DMA chunks */
  es->host_rdma_windows = mx_kmalloc(sizeof(es->host_rdma_windows[0]) 
				    * myri_mx_max_rdma_windows, MX_MZERO);
  if (es->host_rdma_windows == NULL) {
    mx_common_close(es);
    return (ENOMEM);
  }
    
  /* allocate the copyblocks */
  status = mx_alloc_copyblocks(es);
  if (status) {
    MX_WARN(("%s: endpt %d, copyblock alloc failed, status = %d\n", 
	     is->is_name, es->endpt, status));
    mx_common_close(es);
    return (status);
  }

  /* allocate the flow window */
  status = mx_alloc_zeroed_dma_page(is, (char **)&es->flow_window,
				    &es->flow_window_pin);
  if (status) {
    MX_WARN(("%s: endpt %d, flow window allocation failed\n", is->is_name, es->endpt));
    mx_common_close(es);
    return (status);
  }
  mx_reserve_page(es->flow_window);

  status = mx_open_mcp_endpoint(es);
  if (status != 0) {
    MX_WARN(("%s: endpt %d, mx_open_mcp_endpoint failed\n", is->is_name, es->endpt));
    mx_common_close(es);
    return (status);
  }

  status = mx_enable_mcp_endpoint(es);  
  if (status != 0) {
    MX_WARN(("%s: endpt %d, mx_enable_mcp_endpoint failed\n", is->is_name, es->endpt));
    mx_common_close(es);
    return (status);
  }

  es->parity_errors_detected = is->parity_errors_detected;
  es->parity_errors_corrected = is->parity_errors_corrected;

  return 0;
}


mx_endpt_state_t *
mx_get_endpoint(mx_instance_state_t *is, int endpt)
{
  mx_endpt_state_t *es;

  if ((uint32_t) endpt >= is->es_count)
    return NULL;

  mx_mutex_enter(&is->sync);
  es = is->es[endpt];
  if (es == NULL)
    goto abort_with_is;
  mx_mutex_enter(&es->sync);
  if (es->flags & MX_ES_CLOSING) {
    mx_mutex_exit(&es->sync);
    es = NULL;
    goto abort_with_is;
  }
  es->ref_count++;
  mx_mutex_exit(&es->sync);

 abort_with_is:
  mx_mutex_exit(&is->sync);
  return es;
}

void
mx_put_endpoint(mx_endpt_state_t *es)
{
  mx_mutex_enter(&es->sync);
  if ((es->flags & MX_ES_CLOSING) && (es->ref_count == 1))
    mx_wake(&es->sync);
  es->ref_count--;
  mx_mutex_exit(&es->sync);
}

void
mx_common_close(mx_endpt_state_t *es)
{

  mx_instance_state_t *is;
  uint32_t dummy;
  int i, status = 0;
  int close_loop_cnt = 0;
  
  is = es->is;

  /* wait for all references to go away */
 again:
  mx_mutex_enter(&es->sync);
  if (es->flags & MX_ES_RECOVERY) {
    es->flags &= ~MX_ES_RECOVERY;
    mx_wake(&es->is->ready_for_recovery_sync);
  }
  es->flags |= MX_ES_CLOSING;
  if (es->ref_count != 0) {
    if (close_loop_cnt == 0)
      MX_INFO(("%s: closing endpoint %d with %d references, sleeping\n", 
              es->is->is_name, es->endpt, es->ref_count));
    mx_mutex_exit(&es->sync);
    mx_sleep(&es->sync, MX_CLOSE_WAIT, MX_SLEEP_NOINTR);
    close_loop_cnt++;
    goto again;
  }
  mx_mutex_exit(&es->sync);

  if (es->es_type == MYRI_ES_MX)
    status = mx_mcp_command(is, MCP_CMD_CLOSE_ENDPOINT, es->endpt, 0, 0, &dummy);

  if (status != 0)
      MX_WARN(("%s: Failed to close endpoint %d on mcp\n",
	       is->is_name, es->endpt));


  /* sleep for up to 10 seconds waiting for the mcp to close the
     endpoint before we begin to free resources which it may
     depend on
  */
  if (es->es_type == MYRI_ES_MX && status == 0) {
    status = mx_sleep(&es->close_sync, MX_CLOSE_WAIT, MX_SLEEP_NOINTR);
    if (status) {
      MX_WARN(("%s: Timed out waiting for MCP to close endpoint %d\n",
	       is->is_name, es->endpt));
      mx_mark_board_dead(is, MX_DEAD_ENDPOINT_CLOSE_TIMEOUT, es->endpt);
    }
  }

  if (es->es_type == MYRI_ES_RAW) {
    myri_kraw_tx_wait_pending(is);
  }

  mx_mutex_enter(&is->sync);
  if (es->es_type == MYRI_ES_RAW)
    is->raw.es = NULL;
  else {
    mal_always_assert(es->endpt >= 0 && es->endpt < is->es_count);
    is->es[es->endpt] = 0;
  }
  mx_mutex_exit(&is->sync);

  if (es->es_type == MYRI_ES_SNF_RX)
    myri_snf_rx_teardown(es);
  mx_spin_lock_destroy(&es->snf.tx_lock);
  
  if (es->es_type == MYRI_ES_MX) {
    if (es->host_rdma_windows) {
      for (i = 0; i < myri_mx_max_rdma_windows; i++)
	if (es->host_rdma_windows[i].win )
	  mx_deregister(es, i);
      mx_kfree(es->host_rdma_windows);
      es->host_rdma_windows = NULL;
    }
    es->mcp_rdma_windows = 0;

    mx_endpt_free_copyblock(es, &es->sendq);
    mx_endpt_free_huge_copyblock(es, &es->recvq);
    mx_endpt_free_copyblock(es, &es->eventq);
  } else if (es->es_type == MYRI_ES_RAW) {
    myri_kraw_destroy(is);
    mx_mutex_enter(&is->sync);
    is->raw.use_count--;
    mx_mutex_exit(&is->sync);
  }


  if (es->flow_window) {
    mx_unreserve_page(es->flow_window);
    mx_free_dma_page(is, (char **)&es->flow_window, &es->flow_window_pin);
    es->flow_window = NULL;
  }

  mx_sync_destroy(&es->close_sync);
  mx_sync_destroy(&es->wait_sync);
  mx_sync_destroy(&es->app_wait_sync);
  mx_sync_destroy(&es->sync);
  mx_release_instance(is);
}

void
mx_dump_interrupt_ring(mx_instance_state_t *is)
{
  int slot;
  mcp_intr_t *desc;
  
  MX_PRINT(("--------- Dumping interrupt ring state ----- \n"));
  MX_PRINT(("currently expecting interrupts on slot=%d, seq=0x%x\n\n", 
	    is->intr.slot, is->intr.seqnum));
  /*  MX_PRINT(("REQ_ACK_0 = %x\n", mx_read_lanai_special_reg_u32(is, lx.ISR) & MX_LX_REQ_ACK_0));*/
  for (slot = 0; slot < MX_VPAGE_SIZE / sizeof(mcp_intr_t); slot++) {
    desc = &is->intr.ring[slot];
    MX_PRINT(("[%d]: seq=0x%x flags=%d valid=%d\n",
	      slot, ntohs(desc->seqnum), ntohl(desc->flags), desc->valid));
  }
}

int
mx_common_interrupt(mx_instance_state_t *is)
{
  mcp_intr_t *desc;
  uint32_t slot, flags, tmp;
  
  slot = is->intr.slot;
#if MAL_DEBUG
  is->intr.cnt++;
#endif  
  
  /* don't try to handle unless interrupt queues have been setup */
  if (is->intr.ring == 0)
    return 0;
  
  desc = &is->intr.ring[slot];
  
  /* PIO read to wait for desc, if enabled */
  if (myri_irq_sync && !desc->valid) {
    tmp = is->board_ops.read_irq_level(is);
  }
  
  /* check if we have a valid intr descriptor */
  if (!desc->valid) {
    is->intr.spurious++;
    return 0;
  }
  
  /* check interrupt seqnum */
  if (ntohs(desc->seqnum) != is->intr.seqnum) {
    uint16_t cmd;
    
    MX_WARN(("bad interrupt sequence number (slot %d, got %d, expected %d), "
	     "disabling interrupts\n", slot, ntohs(desc->seqnum), 
	     is->intr.seqnum));
    mx_dump_interrupt_ring(is);
    is->board_ops.disable_interrupt(is);
    /* in case we use MSI, disable DMA */
    mx_read_pci_config_16(is, MX_PCI_COMMAND, &cmd);
    mx_write_pci_config_16(is, MX_PCI_COMMAND, cmd & ~MX_PCI_COMMAND_MASTER);
    return 0;
  }
  
  /* claim the interrupt immediately */
  is->board_ops.claim_interrupt(is);
  
  /* process interrupt desc */
  flags = ntohl(desc->flags);
  
  /* if no interrupt flags, get out of here */
  if (!flags) {
    MX_WARN(("%s: slot %d: empty intr desc\n", is->is_name, slot));
    mx_dump_interrupt_ring(is);
    is->board_ops.disable_interrupt(is);
  }
  
  if (flags & MCP_INTR_ETHER_TX)
    mx_ether_tx_done(is, ntohl(desc->ether_tx_cnt));
  
  if (flags & MCP_INTR_ETHER_RX) {
    mx_ether_rx_done(is);
#ifdef HAVE_LRO
    mx_ether_lro_flush(is);
#endif
  }
  
  if (flags & MCP_INTR_ETHER_DOWN)
    mx_wake(&is->ether->down_sync);
  
  if (flags & MCP_INTR_LINK_STATE)
    {
      uint32_t old_state = is->link_state;
      uint32_t nic_state = ntohl(desc->link_state);
      int port = 0;
      const char *state_str;
      
      is->link_state &= ~((MCP_LINK_STATE_UP + MCP_LINK_STATE_MISMATCH) << port);
      is->link_state |= (nic_state << port);
      mx_ether_link_change_notify(is);
      if (nic_state == MCP_LINK_STATE_UP)
	state_str = "UP";
      else if (!(nic_state & MCP_LINK_STATE_UP))
	state_str = "DOWN";
      else
	state_str = "on Wrong network";
      if (old_state != is->link_state)
	MX_INFO(("%s: Link%d is %s\n",is->is_name, port, state_str));
    }
  
  if (flags & MCP_INTR_DMABENCH) {
    is->dmabench.cycles = ntohs(desc->dmabench_cpuc);
    is->dmabench.count = ntohs(desc->dmabench_cnt);
    mx_wake(&is->dmabench.wait_sync);
  }
  
  if (flags & MCP_INTR_LOGGING) {
    is->logging.size = ntohs(desc->logging_size);
    mx_wake(&is->logging.sync);
  }
  
#if 0
  /* FIXME: use INTR_PRINT for 10g */
  if (flags & MCP_INTR_PRINT)
    mx_lanai_print(is, ntohs(desc->print_size));
#endif
  
  if (flags & MCP_INTR_ENDPT)
    {
      mcp_endpt_desc_t *endpt_desc;
      mx_endpt_state_t *es;
      uint32_t i, endpoint, type;
      
      i = is->endpt_desc.index & ((MX_VPAGE_SIZE/sizeof(*endpt_desc)) - 1);
      endpt_desc = &((mcp_endpt_desc_t *)is->endpt_desc.addr)[i];
      
      while (endpt_desc->valid) {
	
	endpt_desc->valid = 0;
	endpoint = endpt_desc->index;
	type = endpt_desc->type;
	
	is->endpt_desc.index++;
	
	if (endpoint >= myri_max_endpoints) {
	  MX_WARN(("%s: intr on invalid endpoint 0x%x\n", 
		   is->is_name, endpoint));
	  break;
	}
	if ((es = is->es[endpoint]) == 0) {
	  MX_WARN(("%s: intr on closed endpoint 0x%x\n", 
		   is->is_name, endpoint));
	  break;
	}
      
	switch (type) {
#if MAL_OS_LINUX
        case MCP_ENDPT_WAKE_TX:
          if (!(es->flags & MX_ES_CLOSING))
          {
            int intr_requested;
            mx_spin_lock(&es->snf.tx_lock);
            intr_requested = es->snf.tx_intr_requested;
            es->snf.tx_intr_requested = 0;
            mx_spin_unlock(&es->snf.tx_lock);
            myri_snf_handle_tx_intr(es);
            if (intr_requested)
	      mx_wake(&es->wait_sync);
          }
          break;
        case MCP_ENDPT_HELPER:
          /* caused by refcount going to 0 in send queue */
          if (!(es->flags & MX_ES_CLOSING))
            myri_snf_handle_tx_intr(es);
          break;
#endif /* MAL_OS_LINUX */
	case MCP_ENDPT_ERROR:
	  MX_WARN(("%s:mcp detected endpoint error on endpoint 0x%x\n", 
		   is->is_name, endpoint));
	  es->endpt_error = MX_WAIT_ENDPT_ERROR;
	  break;
	
	case MCP_ENDPT_CLOSED:
	  mx_wake(&es->close_sync);
	  break;
	
	default:
	  MX_WARN(("%s: unknown endpoint descriptor %d\n", is->is_name, type));
	}

	i = is->endpt_desc.index & ((MX_VPAGE_SIZE/sizeof(*endpt_desc)) - 1);
	endpt_desc = &((mcp_endpt_desc_t *)is->endpt_desc.addr)[i];
      }
    }
  
  if (flags & MCP_INTR_RAW) {
    if (is->raw.es != NULL)
      myri_kraw_intr(is);
  }
  
  
  if (flags & MCP_INTR_SNF_WAKE) {
    if (is->snf.state != MYRI_SNF_RX_S_FREE) {
      myri_snf_rx_state_t *snf = &is->snf;
      mx_endpt_state_t *es;
      int rx_intr_requested;
	
      mx_spin_lock(&snf->rx_intr_lock);
      es = snf->rx_intr_es;
      rx_intr_requested = snf->rx_intr_requested;
      snf->rx_intr_requested = 0;
      mx_spin_unlock(&snf->rx_intr_lock);
      if (rx_intr_requested && es)
        myri_snf_rx_wake(snf, es);
    }
  }
  
  
  desc->valid = 0;
  is->intr.seqnum++;
  is->intr.slot = (slot + 1) & ((MX_VPAGE_SIZE / sizeof(mcp_intr_t)) - 1);
  
  return 1;
}

/* In case compiled outside MX */
#ifndef MX_MAX_SEGMENTS
#define MX_MAX_SEGMENTS 256
#endif

static int
mx_pin(mx_endpt_state_t *es, const uaddr_t in)
{
  int nsegs, status;
  uint32_t handle_id;
  mx_reg_seg_t *segs = NULL;
  mx_reg_t pin_struct, *ps;

  status = mx_copyin(in, &pin_struct, sizeof(pin_struct), es->is_kernel);
  if (status)
    return status;

  ps = &pin_struct;
  nsegs = ps->nsegs;
  handle_id = ps->rdma_id;

  if (nsegs > MX_MAX_SEGMENTS) {
    MX_WARN (("mx_pin: too many segments\n"));
    return E2BIG;
  }

  if (handle_id >= myri_mx_max_rdma_windows) {
    MX_WARN (("MX__PIN: Handle %d too large\n", handle_id));
    return EINVAL;
  }

  if (nsegs > 1) {
    segs = mx_kmalloc(nsegs * sizeof(*segs), 0);
    if (segs == NULL) {
      return ENOMEM;
    }
    status = mx_copyin((uaddr_t) ps->segs.vaddr,
		       segs, nsegs*sizeof(*segs),
		       es->is_kernel);
    if (status) {
      goto abort_with_segs;
    }
    status = mx_register(es, handle_id, ps->seqnum, nsegs, segs, ps->memory_context);
  } else {
    status = mx_register(es, handle_id, ps->seqnum, nsegs, &ps->segs, ps->memory_context);
  }
  /* end fall through the same code used for abort */
  
 abort_with_segs:
  if (segs)
    mx_kfree(segs);
  mal_assert(status != 0 ||
	    MX_PIO_READ(&(es->mcp_rdma_windows[handle_id].table.low)) != MX_DMA_INVALID_ENTRY);
  return status;
}

#if MAL_OS_WINDOWS
#define WINDOWS_BOUNDS_CHECK_COPY(len, limit) do       \
  if (len > limit)                                     \
    {                                                  \
      status = EINVAL;                                 \
      goto abort;                                      \
    }                                                  \
  while (0);
#else
#define WINDOWS_BOUNDS_CHECK_COPY(len, limit)
#endif



int
mx_endptless_ioctl(uint32_t cmd, const uaddr_t in, uint32_t privileged,
		   uint32_t is_kernel)
{
  int status = 0;
  mx_instance_state_t *is;
  uaddr_t out = in;

  privileged |= myri_security_disabled;

  switch (cmd) {

  case MYRI_GET_PORT_COUNT:
    {
      uint32_t board;

      status = mx_copyin(in, &board, sizeof(board), is_kernel);
      if (status)
	goto abort_with_nothing;

      is = mx_get_instance(board);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      status = mx_copyout(&is->num_ports, out, sizeof(is->num_ports), is_kernel);
      mx_release_instance(is);
    }
    break;

  case MYRI_GET_LOGGING:
    {
      myri_get_logging_t x;

      status = mx_copyin(in, &x, sizeof(x), is_kernel);
      if (status)
	goto abort_with_nothing;

      is = mx_get_instance(x.board);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }

      status = mx_get_logging(is, x.size, (uaddr_t) x.buffer, is_kernel);
      mx_release_instance(is);
    }
    break;


  case MYRI_GET_COUNTERS_CNT:
  case MYRI_GET_COUNTERS_STR:
  case MYRI_GET_COUNTERS_VAL:
    {
      uint32_t board, cnt;
      const char **counters;
      
      status = mx_copyin(in, &board, sizeof(board), is_kernel);
      if (status)
	goto abort_with_nothing;

      is = mx_get_instance(board);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }

      status = myri_counters(is->board_type, &counters, &cnt);
      if (status) {
	MX_WARN (("Don't know which counters we have\n"));
	status = ENXIO;
	goto abort_with_is;
      }
      
      if (cmd == MYRI_GET_COUNTERS_CNT) {
	status = mx_copyout(&cnt, out, sizeof(cnt), is_kernel);
      } else if (cmd == MYRI_GET_COUNTERS_STR) {
	char tmp[MYRI_MAX_STR_LEN];
	int i;
	for (i = 0; i < cnt; ++i, out += MYRI_MAX_STR_LEN) {
	  strncpy(tmp, counters[i], MYRI_MAX_STR_LEN-1); 
	  tmp[MYRI_MAX_STR_LEN-1] = '\0';
	  status = mx_copyout(tmp, out, MYRI_MAX_STR_LEN, is_kernel);
	  if (status)
	    goto abort_with_is;
	}
      } else if (cmd == MYRI_GET_COUNTERS_VAL) {
	status = mx_copyout(is->counters.addr, out, cnt * 4, is_kernel);
      } else
	status = ENOTTY;
      
      mx_release_instance(is);
    }
    break;
    
  case MYRI_GET_COUNTERS_IRQ:
    {
      uint32_t board;
      myri_get_counters_irq_t irq;
      
      status = mx_copyin(in, &board, sizeof(board), is_kernel);
      if (status)
	goto abort_with_nothing;

      is = mx_get_instance(board);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      
      irq.count = is->intr.cnt;
      irq.events = is->intr.seqnum;
      irq.spurious = is->intr.spurious;
      status = mx_copyout(&irq, out, sizeof(irq), is_kernel);
      mx_release_instance(is);
    }
    break;
    
  case MYRI_GET_COUNTERS_KB:
    {
      myri_get_counters_kb_t x;

      status = mx_copyin(in, &x, sizeof(x), is_kernel);
      if (status)
	goto abort_with_nothing;

      is = mx_get_instance(x.board);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      
      x.num_ports = 1;
      x.send_kb_0 = ((uint32_t *)is->counters.addr)[2];
      x.recv_kb_0 = ((uint32_t *)is->counters.addr)[3];

      if ((MAL_NUM_PORTS(is->board_type)) > 1) {
	x.num_ports = 2;
	x.recv_kb_0 = ((uint32_t *)is->counters.addr)[4];
	x.send_kb_1 = ((uint32_t *)is->counters.addr)[3];
	x.recv_kb_1 = ((uint32_t *)is->counters.addr)[5];
      }
      
      status = mx_copyout(&x, out, sizeof(x), is_kernel);
      mx_release_instance(is);
    }
    break;

  case MYRI_CLEAR_COUNTERS:
    {
      uint32_t board_num;
      
      status = mx_copyin(in, &board_num, sizeof(board_num), is_kernel);
      if (status)
	goto abort_with_nothing;

      is = mx_get_instance(board_num);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      
      if (!privileged) {
	status = EPERM;
	goto abort_with_is;
      }
      
      status = mx_mcp_command(is, MCP_CMD_RESET_COUNTERS, 0, 0, 0, &board_num);
      myri_snf_clear_stats(is, 0 /* not a snf reset */);
      mx_release_instance(is);
    }
    break;

  case MYRI_GET_BOARD_COUNT: 
    status = mx_copyout(&mx_num_instances, out, sizeof(mx_num_instances), is_kernel);
    break;

  case MYRI_GET_BOARD_MAX:
    status = mx_copyout(&myri_max_instance, out, sizeof(myri_max_instance), is_kernel);
    break;

  case MYRI_GET_ENDPT_MAX:
    {
      uint32_t num_endpoints = myri_max_endpoints;
      num_endpoints = 3*myri_max_endpoints + 1;
      status = mx_copyout(&num_endpoints, out, sizeof(num_endpoints), is_kernel);
    }
    break;
    
  case MX_GET_SMALL_MESSAGE_THRESHOLD:
    status = mx_copyout(&myri_mx_small_message_threshold, out, 
			sizeof(myri_mx_small_message_threshold), is_kernel);
    break;
    
  case MX_GET_MEDIUM_MESSAGE_THRESHOLD:
    status = mx_copyout(&myri_mx_medium_message_threshold, out, 
			sizeof(myri_mx_medium_message_threshold), is_kernel);
    break;

  case MYRI_GET_NIC_ID:
    {
      myri_get_nic_id_t x;
      int i;
      
      status = mx_copyin(in, &x, sizeof(x), is_kernel);
      if (status)
	goto abort_with_nothing;

      is = mx_get_instance(x.board);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }

      x.nic_id = 0;
      for (i = 0; i < 6; i++)
	x.nic_id = (x.nic_id << 8) + is->mac_addr[i];

      status = mx_copyout(&x, out, sizeof(x), is_kernel);
      mx_release_instance(is);
    }
    break;

  case MX_GET_MAX_SEND_HANDLES:
    status = mx_copyout(&myri_mx_max_send_handles, out, 
			sizeof(myri_mx_max_send_handles), is_kernel);
    break;

  case MX_GET_MAX_RDMA_WINDOWS:
    status = mx_copyout(&myri_mx_max_rdma_windows, out, 
			sizeof(myri_mx_max_rdma_windows), is_kernel);
    break;

  case MX_NIC_ID_TO_BOARD_NUM:
    {
      mx_nic_id_to_board_num_t x;
      int i;

      if ((status = mx_copyin(in, &x, sizeof(x), is_kernel))) {
	goto abort_with_nothing;
      }
      mx_mutex_enter(&mx_global_mutex);
      for (i = 0; i < myri_max_instance; ++i) {
	is = mx_instances[i];
	if (!is)
	  continue;
	if ((((uint64_t)is->mac_addr[0] << 40) |
	     ((uint64_t)is->mac_addr[1] << 32) |
	     ((uint64_t)is->mac_addr[2] << 24) |
	     ((uint64_t)is->mac_addr[3] << 16) |
	     ((uint64_t)is->mac_addr[4] <<  8) |
	     ((uint64_t)is->mac_addr[5])) == x.nic_id) {
	  break;
	}
      }
      if (i >= myri_max_instance) {
	status = ENODEV;
	mx_mutex_exit(&mx_global_mutex);
	goto abort_with_nothing;
      }
      mx_mutex_exit(&mx_global_mutex);
      x.board_number = i;
      status = mx_copyout(&x, out, sizeof(x), is_kernel);
    }
    break;


  case MYRI_RAW_GET_PARAMS:
    {
      myri_raw_params_t x;
      
      status = mx_copyin(in, &x, sizeof(x), is_kernel);
      if (status)
	goto abort_with_nothing;
      is = mx_get_instance(x.board_number);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }

      x.raw_mtu = MCP_RAW_MTU;
      x.raw_max_route = MYRI_RAW_MAXROUTE;
      x.raw_num_tx_bufs = MCP_RAW_TX_CNT;
      status = mx_copyout(&x, out, sizeof(x), is_kernel);
      mx_release_instance(is);
    }
    break;

  case MX_GET_SERIAL_NUMBER:
    {
      uint32_t board_number;

      if ((status = mx_copyin(in, &board_number,
			      sizeof(board_number), is_kernel))) {
	goto abort_with_nothing;
      }
      is = mx_get_instance(board_number);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      status = mx_copyout(&is->lanai.serial, out,
			  sizeof(is->lanai.serial), is_kernel);
      mx_release_instance(is);
    }
    break;

  case MYRI_GET_ENDPT_OPENER:
    {
      myri_get_endpt_opener_t x;
      mx_endpt_state_t *es = 0;
      int raw = 0;
      int ether = 0;

      if ((status = mx_copyin(in, &x, sizeof(x), is_kernel)))
	goto abort_with_nothing;

      is = mx_get_instance(x.board);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }

      switch ((int) x.endpt) {
      case -2: 
	ether = 1;
	break;
      case -1:
	raw = 1;
	break;
      default:
	if (x.endpt >= is->es_count) {
	  status = EINVAL;
	  goto abort_with_is;
	}
	break;
      }
      mx_mutex_enter(&is->sync);
      if (raw) {
#if MYRI_ENABLE_KRAW
	es = is->raw.es;
#else
	es = NULL;
#endif
      } else if (ether)
	es = NULL;
      else
	es = is->es[x.endpt];

      x.closed = 0;
      if (es) {
	bcopy(&es->opener, &x.opener, sizeof(x.opener));
      } else if (!ether || !is->ether_is_open) {
	  x.closed = 1;
      }
      mx_mutex_exit(&is->sync);
      status = mx_copyout(&x, out, sizeof(x), is_kernel);
      mx_release_instance(is);
    }
    break;

  case MX_GET_CACHELINE_SIZE:
    {
      status = mx_copyout(&mx_cacheline_size, out, sizeof(mx_cacheline_size), is_kernel);
    }
    break;

  case MX_RUN_DMABENCH:
    {
      mx_dmabench_t x;

      status = mx_copyin(in, &x, sizeof(x), is_kernel);
      if (status)
	goto abort_with_nothing;

      is = mx_get_instance(x.board_number);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      
      status = mx_run_dmabench(is, &x);
      if (!status)
	status = mx_copyout(&x, out, sizeof(x), is_kernel);

      mx_release_instance(is);
    }
    break;
    
    
  case MX_GET_BOARD_STATUS:
    {
      uint32_t board_num, board_status;

      status = mx_copyin(in, &board_num, sizeof(board_num), is_kernel);
      if (status) {
	goto abort_with_nothing;
      }

      is = mx_get_instance(board_num);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      board_status = is->saved_state.reason;
      status = mx_copyout(&board_status, out, sizeof(board_status), is_kernel);
      mx_release_instance(is);
    }
    break;
  case MX_GET_CPU_FREQ:
    {
      uint32_t board_num;

      status = mx_copyin(in, &board_num, sizeof(board_num), is_kernel);
      if (status) {
	goto abort_with_nothing;
      }

      is = mx_get_instance(board_num);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      status = mx_copyout(&is->cpu_freq, out, sizeof(is->cpu_freq), is_kernel);
      mx_release_instance(is);
    }
    break;

  case MX_GET_PCI_FREQ:
    {
      uint32_t board_num;

      status = mx_copyin(in, &board_num, sizeof(board_num), is_kernel);
      if (status) {
	goto abort_with_nothing;
      }

      is = mx_get_instance(board_num);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      status = mx_copyout(&is->pci_freq, out, sizeof(is->pci_freq), is_kernel);
      mx_release_instance(is);
    }
    break;

  case MYRI_GET_INTR_COAL:
  case MYRI_SET_INTR_COAL:
    {
      myri_intr_coal_t x;

      status = mx_copyin(in, &x, sizeof(x), is_kernel);
      if (status)
	goto abort_with_nothing;

      is = mx_get_instance(x.board);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      
      if (cmd == MYRI_SET_INTR_COAL) {
	if (!privileged) {
	  status = EPERM;
	  goto abort_with_is;
	}
	if (x.delay > 1000) {
	  status = ERANGE;
	  goto abort_with_is;
	}
	MCP_SETVAL(is, intr_coal_delay, x.delay);
      } else
	x.delay = MCP_GETVAL(is, intr_coal_delay);

      status = mx_copyout(&x, out, sizeof(x), is_kernel);
      mx_release_instance(is);
    }
    break;
    
  case MX_GET_LINK_STATE:
    {
      uint32_t board_num;

      status = mx_copyin(in, &board_num, sizeof(board_num), is_kernel);
      if (status) {
	goto abort_with_nothing;
      }

      is = mx_get_instance(board_num);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      status = mx_copyout(&is->link_state, out, sizeof(is->link_state), 
			  is_kernel);
      mx_release_instance(is);
    }
    break;
    
  case MX_GET_PRODUCT_CODE:
  case MX_GET_PART_NUMBER:
    {
      mx_get_eeprom_string_t x;
      char *unknown = "unknown";
      char *ptr;
      int len;

      status = mx_copyin(in, &x, sizeof(x), is_kernel);
      if (status) {
	goto abort_with_nothing;
      }

      is = mx_get_instance(x.board_number);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      if (cmd == MX_GET_PRODUCT_CODE)
	ptr = is->lanai.product_code;
      else
	ptr = is->lanai.part_number;

      if (ptr == NULL) {
	ptr = unknown;
      }

      len = strlen(ptr) + 1;
      if (len > MYRI_MAX_STR_LEN)
	len = MYRI_MAX_STR_LEN;

      status = mx_copyout(ptr, (uaddr_t)x.buffer, len, is_kernel);
      mx_release_instance(is);
    }
    break;

  case MYRI_GET_VERSION:
    {
      myri_get_version_t x;
      x.driver_api_magic = MYRI_DRIVER_API_MAGIC;
      strncpy(x.version_str,MYRI_VERSION_STR, sizeof(x.version_str));
      strncpy(x.build_str,MYRI_BUILD_STR, sizeof(x.build_str));
      x.version_str[sizeof(x.version_str) - 1] = 0;
      x.build_str[sizeof(x.build_str) - 1] = 0;
      status = mx_copyout(&x, out, sizeof x, is_kernel);
    }
    break;

  case MYRI_GET_SRAM_SIZE:
  case MYRI_GET_BOARD_TYPE:
  case MYRI_GET_BOARD_NUMA_NODE:
    {
      uint32_t x;
      status = mx_copyin(in, &x, sizeof(x), is_kernel);
      if (status)
	goto abort_with_nothing;

      is = mx_get_instance(x);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      x = ((cmd == MYRI_GET_SRAM_SIZE)  ? is->sram_size :
	   (cmd == MYRI_GET_BOARD_TYPE) ? is->board_type :
	   (cmd == MYRI_GET_BOARD_NUMA_NODE) ? mx_get_board_numa_node(is) :
	   0);
      status = mx_copyout(&x, out, sizeof x, is_kernel);
      mx_release_instance(is);
    }
    break;

  case MYRI_PCI_CFG_READ:
  case MYRI_PCI_CFG_WRITE:
    {
      uint8_t val8;
      uint16_t val16;
      uint32_t val32;
      myri_pci_cfg_t x;

      if (!privileged) {
	status = EPERM;
	goto abort_with_nothing;
      }
      status = mx_copyin(in, &x, sizeof(x), is_kernel);
      if (status)
	goto abort_with_nothing;

      is = mx_get_instance(x.board);
      if (!is) {
	status = ENODEV;
	goto abort_with_nothing;
      }
      if (cmd == MYRI_PCI_CFG_READ) {
	switch (x.width) {
	case 1:
	  status = mx_read_pci_config_8(is, x.offset, &val8);
	  x.val = val8;
	  break;
	case 2:
	  status = mx_read_pci_config_16(is, x.offset, &val16);
	  x.val = val16;
	  break;
	case 4:
	  status = mx_read_pci_config_32(is, x.offset, &val32);
	  x.val = val32;
	  break;
	default:
	  status = EINVAL;
	}
      } else {
	switch (x.width) {
	case 1:
	  status = mx_write_pci_config_8(is, x.offset, x.val);
	  break;
	case 2:
	  status = mx_write_pci_config_16(is, x.offset, x.val);
	  break;
	case 4:
	  status = mx_write_pci_config_32(is, x.offset, x.val);
	  break;
	default:
	  status = EINVAL;
	}
      }
      if (!status)
	status = mx_copyout(&x, out, sizeof x, is_kernel);
      mx_release_instance(is);
    }
    break;
#if MYRI_ENABLE_PTP
  case MYRI_PTP_RX_AVAIL:
  case MYRI_PTP_GET_RX_MSG:
  case MYRI_PTP_STORE_RX_MSG:
  case MYRI_PTP_START_STOP:
  case MYRI_PTP_PUT_TX_TIMESTAMP:
  case MYRI_PTP_GET_TX_TIMESTAMP:
    {
      status = myri_ptp_ioctl(cmd, in, is_kernel);
    }
    break;
#endif

  default:
    status = ENOTTY;
  }
  
  return status;

 abort_with_is:
  mx_release_instance(is);
 abort_with_nothing:
  return status;
}

/* returns a wait status depending on what happened to the endpoint while sleeping */
static uint32_t
mx_wait_check_endpt_status(mx_endpt_state_t *es)
{
  mx_instance_state_t * is = es->is;
  uint32_t status = MX_WAIT_STATUS_GOOD;
  if (es->parity_errors_detected != is->parity_errors_detected) {
    /* new parity error? */
    es->parity_errors_detected = is->parity_errors_detected;
    status = MX_WAIT_PARITY_ERROR_DETECTED;
    mal_always_assert(es->parity_errors_corrected == is->parity_errors_corrected);
  } else {
    /* endpoint did something bad, or there was an uncorrectable
       parity error which only affects this endpoint */
    status = es->endpt_error;
    es->endpt_error = 0;
  }
  return status;
}

int
mx_common_ioctl(mx_endpt_state_t *es, uint32_t cmd, const uaddr_t in)
{
  uaddr_t out = in;
  int status = 0;

  if (mx_is_dead(es->is)) {
    /* if the board is dead, return an error to the application...
     * except in case of parity since the recovery is notified to the
     * application through the wait_status or get_board_status
     * (other ioctls can still be processed safely since they do not
     * touch the board for anything that won't be redone a afterwards)
     */
    if (!(es->is->flags & MX_PARITY_RECOVERY)) {
      MX_WARN(("firmware dead on board %d, ignoring ioctl\n", es->is->id));
      return EIO;
    }
  }

  switch (cmd) {

  case MX_REGISTER:
    status = mx_pin(es, in);
    break;

  case MX_DEREGISTER:
    {
      uint32_t handle;
      status = mx_copyin(in, &handle, sizeof(handle), es->is_kernel);
      if (status) {
	goto abort;
      }
      
      if (handle >= myri_mx_max_rdma_windows) {
	MX_WARN (("MX_DEREGISTER: Handle %d too large\n", handle));
	status = EINVAL;
	goto abort;
      }
      status = mx_deregister(es, handle);
    }
    break;

  case MX_WAIT:
    /* we try to copy in a 32-bit opaque handle */
    {
      mx_wait_t x;
      int sleep_status;

      status = mx_copyin(in, &x, sizeof(x), es->is_kernel);
      if (status) {
	goto abort;
      }
      mx_mutex_enter(&es->sync);
      es->num_waiters++;
      if (!es->progression_timeout)
	es->progression_timeout = x.timeout;
      do {
	unsigned time_slot = es->progression_timeout;
	if (time_slot >= 50)
	  time_slot = 50;
	es->progression_timeout -= time_slot;
	mx_mutex_exit(&es->sync);
	sleep_status = mx_sleep(&es->wait_sync, time_slot, MX_SLEEP_INTR);
	mx_mutex_enter(&es->sync);
      } while (sleep_status == EAGAIN && es->progression_timeout);
      es->progression_timeout = 0;
      x.mcp_wake_events = 0;
      if (sleep_status == 0 && mx_atomic_read(&es->no_mcp_req_wakeups) == 0) {
	x.mcp_wake_events = 1;
      } else if (sleep_status == 0) {
	mx_atomic_subtract(1, &es->no_mcp_req_wakeups);
      }
      /* check if something bad happened while sleeping */
      x.status = mx_wait_check_endpt_status(es);
      es->num_waiters--;
      mx_mutex_exit(&es->sync);
      status = mx_copyout(&x, out, sizeof(x), es->is_kernel);
    }
    break;

  case MX_APP_WAIT:
    {
      mx_wait_t x;
      int sleep_status;

      status = mx_copyin(in, &x, sizeof(x), es->is_kernel);
      if (status) {
	goto abort;
      }
      mx_mutex_enter(&es->sync);
      es->num_waiters++;
      es->app_waiting = 1;
      mx_mutex_exit(&es->sync);
      sleep_status = mx_sleep(&es->app_wait_sync, x.timeout, MX_SLEEP_INTR);
      mx_mutex_enter(&es->sync);
      x.mcp_wake_events = 0;
      if (sleep_status == 0 && mx_atomic_read(&es->no_mcp_req_wakeups) == 0) {
	x.mcp_wake_events = 1;
      } else if (sleep_status == 0) {
	mx_atomic_subtract(1, &es->no_mcp_req_wakeups);
      }
      /* check if something bad happened while sleeping */
      x.status = mx_wait_check_endpt_status(es);
      if (x.status == MX_WAIT_STATUS_GOOD && sleep_status != 0)
	x.status = MX_WAIT_TIMEOUT_OR_INTR;
      es->app_waiting = 0;
      es->num_waiters--;
      mx_mutex_exit(&es->sync);
      status = mx_copyout(&x, out, sizeof(x), es->is_kernel);
    }
    break;

  case MX_CLEAR_WAIT:
    {
      mx_mutex_enter(&es->sync);
      if (es->num_waiters != 0) {
	status = EBUSY;
      } else {
	mx_sync_reset(&es->wait_sync);
      }
      mx_mutex_exit(&es->sync);
    } break;

  case MX_WAKE:
    {
      /* used by an exiting application to alert the
	 progression thread */
	 
      mx_atomic_add(1, &es->no_mcp_req_wakeups);
      mx_wake(&es->wait_sync);
    } break;

  case MX_APP_WAKE:
    {
      /* used by progression thread to wake application 
	 thread */
      mx_atomic_add(1, &es->no_mcp_req_wakeups);
      es->app_waiting = 0;
      mx_wake(&es->app_wait_sync);
    } break;

  case MX_GET_COPYBLOCKS:
    {
      mx_get_copyblock_t desc;

      desc.sendq_offset = 0;
      desc.sendq_len = es->sendq.size;

      desc.recvq_offset = desc.sendq_offset + es->sendq.size;
      desc.recvq_len = es->recvq.size;

      desc.eventq_offset = desc.recvq_offset + es->recvq.size;
      desc.eventq_len = es->eventq.size;

      desc.user_mmapped_sram_offset = desc.eventq_offset + es->eventq.size;
      desc.user_mmapped_sram_len = es->user_mmapped_sram.size;
      desc.user_mmapped_zreq_offset = (desc.user_mmapped_sram_offset + 	
				       es->user_mmapped_sram.size);
      desc.user_mmapped_zreq_len = es->user_mmapped_zreq.size;

      desc.kernel_window_offset = (desc.user_mmapped_zreq_offset +
                                   desc.user_mmapped_zreq_len);
      desc.kernel_window_len = (es->is->kernel_window ? MX_PAGE_SIZE : 0);

      desc.flow_window_offset = (desc.kernel_window_offset +
				 desc.kernel_window_len);
      desc.flow_window_len = (es->flow_window ? MX_PAGE_SIZE : 0);

      desc.send_compcnt_offset = (desc.flow_window_offset +
                                  desc.flow_window_len);
      desc.send_compcnt_len = (es->is->tx_done.addr ? MX_PAGE_SIZE : 0);

      /* offsets within sram */
      desc.user_reqq_offset = (es->endpt * MCP_UMMAP_SIZE) % MX_PAGE_SIZE;
      desc.user_reqq_len = MCP_UREQQ_CNT * sizeof(mcp_ureq_t);
      desc.user_dataq_offset = desc.user_reqq_len + desc.user_reqq_offset;
      desc.user_dataq_len = MCP_UDATAQ_SIZE;
      
      status = mx_copyout(&desc, out, sizeof(desc), es->is_kernel);
    }
    break;

  case MX_RECOVER_ENDPOINT:
    {
      status = mx_enable_mcp_endpoint(es);
    }
    break;

#if MYRI_ENABLE_KRAW
  case MYRI_RAW_GET_NEXT_EVENT:
    {
      myri_raw_next_event_t x;

      if (es->es_type != MYRI_ES_RAW) {
	status = EPERM;
	goto abort;
      }
      if ((status = mx_copyin(in, &x, sizeof(x), es->is_kernel)))
	  goto abort;
      status = myri_kraw_next_event(es, &x);
      if (!status)
	status = mx_copyout(&x, out, sizeof(x), es->is_kernel);
    }
    break;
  case MYRI_RAW_SEND:
    {
      myri_raw_send_t x;

      if (es->es_type != MYRI_ES_RAW) {
	status = EPERM;
	goto abort;
      }
      if ((status = mx_copyin(in, &x, sizeof(x), es->is_kernel)))
	  goto abort;
      status = myri_kraw_send(es, &x);
    }
    break;
#endif


  case MX_DIRECT_GET:
    {
      mx_direct_get_t xget;
      mx_endpt_state_t *es2;
      mx_shm_seg_t src_seg, dst_seg;

      if ((status = mx_copyin(in, &xget, sizeof(xget), es->is_kernel)))
	goto abort;
      es2 = mx_get_endpoint(es->is, xget.src_endpt);
      if (!es2) {
	status = ESRCH;
	goto abort;
      }
      /* do not check src_session since the MX_DIRECT_GET ABI has been
       * distributed without the lib setting src_session and the driver
       * checking it.
       */

      src_seg.vaddr = xget.src_va;
      src_seg.len = xget.length;
      dst_seg.vaddr = xget.dst_va;
      dst_seg.len = xget.length;

      status = mx_direct_get(es, &dst_seg, 1, es2, &src_seg, 1, xget.length);
      mx_put_endpoint(es2);
      if (status)
	goto abort;
    }
    break;

  case MX_DIRECT_GETV:
    {
      mx_direct_getv_t xget;
      mx_endpt_state_t *es2;

      if ((status = mx_copyin(in, &xget, sizeof(xget), es->is_kernel)))
	goto abort;

      if (xget.dst_nsegs > MX_MAX_SEGMENTS || xget.src_nsegs > MX_MAX_SEGMENTS) {
	status = E2BIG;
	goto abort;
      }	

      es2 = mx_get_endpoint(es->is, xget.src_endpt);
      if (!es2) {
	status = ESRCH;
	goto abort;
      }
      if (es2->session_id != xget.src_session) {
	status = EPERM;
	mx_put_endpoint(es2);
	goto abort;
      }

      status = mx_direct_get(es, &xget.dst_segs, xget.dst_nsegs,
			     es2, &xget.src_segs, xget.src_nsegs, xget.length);
      mx_put_endpoint(es2);
      if (status)
	goto abort;
    }
    break;

  case MX_WAKE_ENDPOINT:
    {
      mx_wake_endpt_t xwake;
      mx_endpt_state_t *es2;
      if ((status = mx_copyin(in, &xwake, sizeof(xwake), es->is_kernel)))
	  goto abort;
      es2 = mx_get_endpoint(es->is, xwake.endpt);
      if (!es2) {
	status = ESRCH;
	goto abort;
      }
      /* no overflow possible because the number of 
	 no_mcp_req_wakeup is bounded by the shmem queue size */
      mx_atomic_add(1, &es2->no_mcp_req_wakeups);
      if (es2->app_waiting) {
	es2->app_waiting = 0;
	mx_wake(&es2->app_wait_sync);
      } else {
	mx_wake(&es2->wait_sync);
      }
      mx_put_endpoint(es2);
    }
    break;

#if MAL_OS_UDRV
  case MYRI_UDRV_DOORBELL:
    {
      myri_udrv_doorbell_t x;
      mx_copyblock_t *cb;

      status = mx_copyin(in, &x, sizeof(x), es->is_kernel);
      if (status)
	goto abort;

      cb = (MAL_IS_ZE_BOARD(es->is->board_type) ?
	    &es->user_mmapped_zreq : &es->user_mmapped_sram);
      mal_always_assert(x.offset >= 0 && x.len <= 64 &&
			x.offset + x.len <= cb->size);
      mx_pio_memcpy((char *)cb->addr + x.offset, x.data, x.len, 0);
    }
    break;
#endif

  case MX_ARM_TIMER:
    {
      uint32_t ms;

      status = mx_copyin(in, &ms, sizeof(ms), es->is_kernel);
      if (status) {
	goto abort;
      }
      mx_mutex_enter(&es->sync);
      es->progression_timeout = ms;
      mx_mutex_exit(&es->sync);
    }
    break;

  case MX_WAIT_FOR_RECOVERY:
    {
      uint32_t status = MX_WAIT_PARITY_ERROR_UNCORRECTABLE;

      mx_sync_init(&es->recovery_sync, es->is, es->endpt, "es->recovery_sync");

      /* notify the watchdog that we are ready */
      mx_wake(&es->is->ready_for_recovery_sync);

      /* wait until recovery is done */
      mx_sleep(&es->recovery_sync, MX_MAX_WAIT, 0);

      if (es->parity_errors_corrected != es->is->parity_errors_corrected)
	status = MX_WAIT_PARITY_ERROR_CORRECTED;
      es->parity_errors_corrected = es->is->parity_errors_corrected;

      mx_sync_destroy(&es->recovery_sync);

      mal_assert(!(es->flags & MX_ES_RECOVERY));

      mx_copyout(&status, out, sizeof(status), es->is_kernel);
    }
    break;

  case MX_SHMEM_EN:
    {
#if MAL_OS_LINUX
      uint32_t val;
      
      status = mx_copyin(in, &val, sizeof(val), es->is_kernel);
      if (status) {
	goto abort;
      }
      status = mx_shmem_thread_handle(es, val);
#endif
    }
    break;

  case MYRI_SNF_WAIT:
    {
      uint32_t x;
      status = mx_copyin(in, &x, sizeof(x), es->is_kernel);
      if (status)
        break;

      /* WAIT_TX-only on SNF's tx-only endpoints */
      status = myri_poll_wait(es, x, MYRI_WAIT_TX_EVENT);
    }
    break;

  case MYRI_SNF_STATS:
    {
      myri_snf_stats_t x;
      myri_snf_get_stats(es->is, &x);
      status = mx_copyout(&x, out, sizeof(x), es->is_kernel);
    }
    break;
  default:
    status = ENOTTY;
  }

 abort:
  return status;
}


int
mx_instance_status_string(int unit, char **str, int *len)
{
  char *c;
  char mcp_status_string[64];
  mx_instance_state_t *is;
  uint32_t mcp_status;
  int ret = 0;

  is = mx_get_instance(unit);
  if (!is) {
    ret = ENODEV;
    goto abort_with_nothing;
  }

  c = mx_kmalloc(512, MX_MZERO|MX_WAITOK);
  if (c == NULL) {
    ret = ENOMEM;
    goto abort_with_is;
  }

  if (mx_is_dead(is)) {
    mcp_status = MCP_GETVAL(is, mcp_status);
    sprintf(mcp_status_string, "not responding (%d,%d,0x%x)", 
	    is->saved_state.reason, is->saved_state.arg, mcp_status);
  } else {
    sprintf(mcp_status_string, "running)");
  }

  sprintf(c, "%s: Driver %s (0x%x), status:%s, uptime %ld, %d ports\n",
	  is->is_name, MYRI_DRIVER_STR, MYRI_DRIVER_API_MAGIC, mcp_status_string, 
	  (long)ntohl(*(uint32_t *)is->counters.addr), is->num_ports);

  *len = (int)strlen(c);
  *str = c;
  mx_release_instance(is);
  return 0;


 abort_with_is:
  mx_release_instance(is);
 abort_with_nothing:
  return ret;
}

int
mx_direct_get_common (mx_shm_seg_t *dst_segs, uint32_t dst_nsegs,
		      void * src_space, mx_shm_seg_t *src_segs, uint32_t src_nsegs,
		      uint32_t length)
{
  mx_shm_seg_t *current_src_seg, *current_dst_seg;
  uint32_t current_src_index, current_dst_index;
  uint32_t current_src_offset, current_dst_offset;
  int status;

  /* start with first src segment */
  current_src_index = 0;
  current_src_seg = &src_segs[0];
  current_src_offset = 0;
  /* start with first dst segment */
  current_dst_index = 0;
  current_dst_seg = &dst_segs[0];
  current_dst_offset = 0;

  while (current_src_index < src_nsegs && current_dst_index < dst_nsegs
	 && length > 0) {
    uint32_t chunk = length;
    /* copy until one segment ends */
    chunk = chunk < current_src_seg->len - current_src_offset
      ? chunk : current_src_seg->len - current_src_offset;
    chunk = chunk < current_dst_seg->len - current_dst_offset
      ? chunk : current_dst_seg->len - current_dst_offset;

    if (chunk) {
      status = mx_arch_copy_user_to_user(current_dst_seg->vaddr + current_dst_offset,
					 current_src_seg->vaddr + current_src_offset,
					 src_space,
					 chunk);
      if (status)
	return status;
    }

    /* update src status */
    current_src_offset += chunk;
    if (current_src_offset == current_src_seg->len) {
      current_src_seg++;
      current_src_index++;
      current_src_offset = 0;
    }
    /* update dst status */
    current_dst_offset += chunk;
    if (current_dst_offset == current_dst_seg->len) {
      current_dst_seg++;
      current_dst_index++;
      current_dst_offset = 0;
    }

    length -= chunk;
  }

  return 0;
}


int
mx_strcasecmp(const char *a, const char *b)
{
  int ca, cb;
  while (*a || *b) {
    ca = *a;
    cb = *b;
    if (ca >= 'a' && ca <= 'z')
      ca -= 'a' - 'A';
    if (cb >= 'a' && cb <= 'z')
      cb -= 'a' - 'A';
    if (ca != cb)
      return ca - cb;
    a++;
    b++;
  }
  return 0;
}

/*
 * Clock synchronization between NIC and host
 */
static uint64_t myri_read_hwclock(mx_instance_state_t *is);

int
myri_clock_init(mx_instance_state_t *is)
{
  struct timespec ts;
  uint32_t offset;
  int status = 0;

  status = mx_mcp_command(is, MCP_CMD_GET_HWCLOCK_OFFSET,
			  0, 0, 0, &offset);
  if (status) /* no HW clock on this firmware */
    return status;

  is->clk_db_offset = offset;
  mx_spin_lock_init(&is->clk_spinlock, is, -1, "clock spinlock");

  /* We use the last 16 bytes of the counters page to read clock updates.
   * There is a check in the counters code to ensure this never gets clobbered
   */
  mal_assert(is->counters.addr);
  is->clk_update = (mcp_hwclock_update_t *)
    (is->counters.addr + MX_VPAGE_SIZE - sizeof(mcp_hwclock_update_t));
  is->clk_nticks.ticks_mask = -1;
  is->clk_nticks.nsecs_per_tick = 500ull;
  mx_preempt_disable();
  mx_getnstimeofday(&ts);
  is->clk_nticks.ticks = myri_read_hwclock(is);
  mx_preempt_enable();
  is->clk_nticks.nsecs = mx_timespec_to_ns(&ts);

  /* Start off by assuming no drift, and default to syncing every second. */
  is->clk_sync_init_cnt = 1;
  is->clk_sync_init_nsecs = is->clk_nticks.nsecs;
  is->clkp_latest.error_nsecs = 0;
  is->clkp_latest.hostnic_drift_nsecs = 0;
  is->clkp_latest.multiplier = 0;
  is->clkp_latest.nsecs_base = is->clk_nticks.nsecs;
  is->kernel_window->clksync.clkp = is->clkp_latest;
  is->clk_initialized = 1;

  return 0;
}

void
myri_clock_destroy(mx_instance_state_t *is)
{
  if (!is->clk_initialized)
    return;
  mx_spin_lock_destroy(&is->clk_spinlock);
  is->clk_initialized = 0;
}

/*
 * Must hold the clk_spinlock as there can only be one HWCLOCK request at a
 * time
 */
static uint64_t
myri_read_hwclock(mx_instance_state_t *is)
{
  uint64_t nic_ticks;
  uint32_t seqno;
  int iters = 0;

  seqno = ++is->clk_hwclock_seqno;
  MX_PIO_WRITE((uint32_t *)(is->lanai.sram + is->clk_db_offset), seqno);
  MAL_STBAR();
  while (is->clk_update->nticks_seqnum != seqno) {
    if (iters++ > 1000000) {
      MX_PRINT_ONCE(("Missed receive hwclock update!\n"));
      break;
    }
    MAL_READBAR();
  }
  nic_ticks = ((uint64_t) ntohl(is->clk_update->nticks_high) << 32) +
              ((uint64_t) ntohl(is->clk_update->nticks_low));

  return nic_ticks;
}

/*
 * Every so often, we sync the host and NIC clocks.
 */
static int
myri_sync_hwclock(mx_instance_state_t *is,
                  uint64_t *host_nsecs_sync,
                  uint64_t *nic_nsecs_sync,
                  int64_t *hnic_drift_nsecs_o)
{
#define CLOCK_NTRIALS 8
#define CLOCK_KEEP 5 /* leave out worst 3 outliers */
  int i, cnt = 0;
  struct {
    int64_t host_delay_nsecs;
    int64_t hnic_drift_nsecs;
  } trials[CLOCK_NTRIALS], trial;

  struct timespec tsB, tsE;
  uint64_t tsB_nsecs, tsE_nsecs, tsAvg_nsecs, nic_nsecs;
  uint64_t tsH_begin = 0, tsN_begin = 0;
  uint64_t nic_ticks;
  int64_t drift_sum = 0;
  unsigned long flags;

  flags = 0;  /* useless initialization to pacify -Wunused */

  for (cnt = 0; cnt < CLOCK_NTRIALS; cnt++) {
    if (mx_is_dead(is))
      return -EIO;
    mx_spin_lock_irqsave(&is->clk_spinlock, flags);
    mx_getnstimeofday(&tsB);
    nic_ticks = myri_read_hwclock(is);
    mx_getnstimeofday(&tsE);
    nic_nsecs = myri_clksync_nticks_update(&is->clk_nticks, nic_ticks);
    mx_spin_unlock_irqrestore(&is->clk_spinlock, flags);

    tsB_nsecs = mx_timespec_to_ns(&tsB);
    tsE_nsecs = mx_timespec_to_ns(&tsE);
    tsAvg_nsecs = (tsB_nsecs + tsE_nsecs) / 2;

    if (cnt == 0) {
      tsN_begin = nic_nsecs;
      tsH_begin = tsAvg_nsecs;
    }

    if (tsB_nsecs > tsE_nsecs) {
      static int once = 0;
      if (!once) {
        MX_WARN(("system clock going backwards? Can't sync host/NIC clocks\n"));
        once = 1;
      }
      return -EIO;
    }
    trial.host_delay_nsecs = (int64_t) tsE_nsecs - tsB_nsecs;
    trial.hnic_drift_nsecs = (int64_t) tsAvg_nsecs - nic_nsecs;

    i = cnt - 1;
    while (i >= 0) {
      if (trials[i].host_delay_nsecs < trial.host_delay_nsecs)
        break;
      trials[i+1] = trials[i];
      i--;
    }
    trials[i+1] = trial;
  }

  /* The middle trial is where host and nic time should line up on average */
  *nic_nsecs_sync = (tsN_begin + nic_nsecs) / 2;
  *host_nsecs_sync = (tsH_begin + tsAvg_nsecs) / 2;

  cnt = CLOCK_KEEP;
  for (i = 0; i < cnt; i++)
    drift_sum += trials[i].hnic_drift_nsecs;
  *hnic_drift_nsecs_o = drift_sum / cnt;
  return 0;
}

static
void
myri_clock_handle_error(mx_instance_state_t *is, uint64_t host_nsecs,
                        int64_t error)
{
  if (is->clk_sync_init_cnt == 0) {
    myri_clksync_error_count++;
    if (myri_clksync_verbose)
      MX_INFO(("%s: clksync offset=%+6"PRId64" nsecs (elapsed=%d secs)\n",
          is->is_name, error,
          (int)((host_nsecs - is->clk_sync_init_nsecs) / 1000000000ll)));
  }
  else {
    /* During initialization */
    const int64_t max_error = 5000;
    const int noerror_cnt = 3;
    const uint64_t max_delay_msecs = 10000ull; /* 10 secs */

    if (MAL_ABS64(error) < max_error) {
      if (++is->clk_sync_init_cnt == noerror_cnt+1) {
        if (myri_clksync_verbose)
          MX_INFO(("%s: clksync init to %lld\n", is->is_name,
                  (long long) error));
        is->clk_sync_init_cnt = 0; /* we're done */
      }
    }
    else {
      uint64_t delay = (host_nsecs - is->clk_sync_init_nsecs) / 1000000ull;
      is->clk_sync_init_cnt = 1; /* start over, need init_error_cnt in a row */
      if (delay >= max_delay_msecs && myri_clksync_verbose) {
        MX_INFO(("%s: clksync init offset=%+6"PRId64" nsecs after %5d msecs\n",
          is->is_name, error, (int) delay));
      }
    }
  }
}

int
myri_clock_sync_now(mx_instance_state_t *is)
{
  int64_t hnic_drift_nsecs, sync_interval;
  int64_t multiplier, off_delta_nsecs, error;
  uint64_t nic_nsecs, host_nsecs;
  unsigned long flags;

  flags = 0;  /* useless initialization to pacify -Wunused */

  if (!is->clk_initialized)
    return 0;

  if (myri_sync_hwclock(is, &host_nsecs, &nic_nsecs, &hnic_drift_nsecs))
    return 0;

  off_delta_nsecs = hnic_drift_nsecs - is->clkp_latest.hostnic_drift_nsecs;
  sync_interval = (int64_t) nic_nsecs - is->clkp_latest.nsecs_base;

  if (sync_interval >= 15000) {
    /* Ensure our sync interval is large enough */
    multiplier = (off_delta_nsecs << MYRI_CLKSYNC_MULTIPLIER_SHIFT)
                  / sync_interval;
    error = is->clkp_latest.error_nsecs = (int64_t)
      host_nsecs - myri_clksync_convert_nsecs(&is->kernel_window->clksync, nic_nsecs);

    if (is->clk_sync_init_cnt || MAL_ABS64(error) >= (int64_t) myri_clksync_error)
      myri_clock_handle_error(is, host_nsecs, error);

    /* Update the "latest clock" using 75% the new multiplier and 25% the
     * previous multiplier */
    is->clkp_latest.multiplier =
      (3 * multiplier + is->clkp_latest.multiplier) / 4;
    is->clkp_latest.nsecs_base = nic_nsecs;
    is->clkp_latest.hostnic_drift_nsecs = hnic_drift_nsecs;

    mx_spin_lock_irqsave(&is->clk_spinlock, flags);
    is->kernel_window->clksync.seqnum++;
    MAL_WRITEBAR();
    is->kernel_window->clksync.clkp = is->clkp_latest;
    MAL_WRITEBAR();
    is->kernel_window->clksync.seqnum++;
    mx_spin_unlock_irqrestore(&is->clk_spinlock, flags);
  }
  return 0;
}
