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

#include "mx_arch.h"
#include "mx_instance.h"
#include "mx_malloc.h"
#include "mx_misc.h"
#include "mx_pio.h"
#include "mcp_config.h"
#include "mal_stbar.h"


/* Hack to avoid clashes between system header files and zlib.h on
   for the declaration of crc32() on some OSes */
#define MX_INSTANCE_C
#ifdef EXPORT
#undef EXPORT
#endif


/* put the board in reset via PCI config space */
static void
mx_board_reset_on(mx_instance_state_t *is)
{
  mx_write_lanai_ctrl_bit(is, MX_LX_CTRL_IO_RESET_ON_BIT);
  MAL_STBAR();
}

/* put the LANai in reset via PCI config space */
static void
mx_lanai_reset_on(mx_instance_state_t *is)
{
  mx_write_lanai_ctrl_bit(is,  MX_LX_CTRL_CPU_RESET_ON_BIT);
  MAL_STBAR();
}

/* take the board out of reset via PCI config space */
static void
mx_board_reset_off(mx_instance_state_t *is)
{
  mx_write_lanai_ctrl_bit(is, MX_LX_CTRL_IO_RESET_OFF_BIT);
  MAL_STBAR();
}

/* take the LANai out of reset via PCI config space */
static void
mx_lanai_reset_off(mx_instance_state_t *is)
{
  mx_write_lanai_ctrl_bit(is,  MX_LX_CTRL_CPU_RESET_OFF_BIT);
  MAL_STBAR();
}

/* enable interrupts */
static void
mx_lx_int_enable(mx_instance_state_t *is)
{
  mx_write_lanai_ctrl_bit(is, MX_LX_CTRL_PCI_INT_ENABLE_ON_BIT);
  MAL_STBAR();
}

/* disable interrupts */
static void
mx_lx_int_disable(mx_instance_state_t *is)
{
  mx_write_lanai_ctrl_bit(is, MX_LX_CTRL_PCI_INT_ENABLE_OFF_BIT);
  MAL_STBAR();
}

static void
mx_lx_park_board(mx_instance_state_t *is)
{
  /* Park the board so that the CPU is frozen, and the hardware
     will drop packets */

  mx_lanai_reset_on(is);
  mx_board_reset_on(is);
  mx_spin(20000);
  mx_board_reset_off(is);

}

static int
mx_lx_detect_parity(mx_instance_state_t *is)
{
  if (mx_read_lanai_special_reg_u32(is, lx.ISR) & MX_LX_PARITY_INT) {
    MX_WARN(("%s: NIC has SRAM Parity Error\n", is->is_name));
    return 1;
  }
  return 0;
}

#ifndef MX_ARCH_ZLIB_INFLATE
#include "zlib.h"

#define mx_zlib_inflate inflate
#define mx_zlib_inflateEnd inflateEnd

static voidpf
mx_zlib_calloc(voidpf opaque, uInt items, uInt size)
{
  return mx_kmalloc(items * size, MX_MZERO|MX_WAITOK);
}

static void
mx_zlib_free(voidpf opaque, voidpf addr)
{
  mx_kfree(addr);
}

static inline int
mx_zlib_inflateInit(z_stream *zs)
{
  zs->zalloc = mx_zlib_calloc;
  zs->zfree = mx_zlib_free;
  zs->opaque = 0;
  return inflateInit(zs);
}
#endif /* MX_ARCH_ZLIB_INFLATE */

int
mx_load_mcp(mx_instance_state_t *is, void *inflate_buffer, int limit, int *actual_len)
{
  z_stream zs;
  int rv, status;
  const unsigned char *mcp;
  int mcp_len;
  unsigned int parity_critical_start, parity_critical_end;

  status = 0;

  status = myri_select_mcp(is->board_type, &mcp, &mcp_len,
			   &parity_critical_start, &parity_critical_end);
  if (status || mcp == NULL) {
    MX_WARN(("%s: Could not find mcp for type %d card (rev %d)\n", 
	     is->is_name, is->board_type, is->pci_rev));
    return ENODEV;
  }

  is->parity_critical_start = (uintptr_t)&is->lanai.sram[parity_critical_start];
  is->parity_critical_end = (uintptr_t)&is->lanai.sram[parity_critical_end];

  /* align them on 8 byte boundaries */
  is->parity_critical_start &= ~(uintptr_t)7;
  is->parity_critical_end = (is->parity_critical_end + (uintptr_t)7) & (uintptr_t)~7;

  rv = mx_zlib_inflateInit(&zs);
  if (rv != Z_OK) {
    MX_WARN(("Fatal error %d in inflateInit\n", rv));
    return ENXIO;
  }

  zs.next_in = (unsigned char *)mcp;
  zs.avail_in = mcp_len;
  zs.next_out = (unsigned char *)inflate_buffer;
  zs.avail_out = limit;

  rv = mx_zlib_inflate(&zs, Z_FINISH);
  if (rv != Z_STREAM_END || zs.avail_in != 0 || zs.avail_out == 0) {
    MX_PRINT(("fatal error in inflate - mcp too big   err %d?\n", rv));
    MX_PRINT(("   lanai sram space originally    = %d, limit=%d\n",
	       is->sram_size, limit));
    MX_PRINT(("   bytes remaining to uncompress  = %d\n",
	       zs.avail_in));
    MX_PRINT(("   Available space for decompress = %d\n",
	       zs.avail_out));
    MX_PRINT(("   inflate error msg %s\n", zs.msg));
    MX_PRINT(("   inflate error char %x\n",
	       *(unsigned char *)zs.next_in));
    MX_PRINT(("   inflate error char %x\n",
	       *(unsigned char *)(zs.next_in - 1)));
    MX_PRINT(("   inflate error char %x\n",
	       *(unsigned char *)(zs.next_in + 1)));
    MX_PRINT(("   inflate error total %ld\n", zs.total_in));
    status = ENXIO;
    goto abort_with_inflate_init;
  }

  /* Make sure we don't copy more than was uncompressed. */
  if (zs.total_out < limit)
    limit = zs.total_out;

  *actual_len = limit;
  mx_zlib_inflateEnd(&zs);
  return 0;

 abort_with_inflate_init:
  mx_zlib_inflateEnd(&zs);

  return status;
  
}

static void
mx_lx_check_parity(mx_instance_state_t *is, const char *stage)
{
  if (mx_read_lanai_special_reg_u32(is, lx.ISR) & MX_LX_PARITY_INT) {
    mx_write_lanai_isr(is, MX_LX_PARITY_INT);
    MX_WARN(("%s, parity was set and was cleared\n", stage));
  }
}

static int
mx_lx_init_board(mx_instance_state_t *is)
{
  volatile char *eeprom_strings;
  int usable_sram_len, status;
  int mcp_len;
  void *img;
  const void *from;

  usable_sram_len = is->sram_size - MX_EEPROM_STRINGS_LEN;

  MAL_DEBUG_PRINT (MAL_DEBUG_BOARD_INIT, ("Putting board into reset\n"));
  mx_board_reset_on(is);

  MAL_DEBUG_PRINT (MAL_DEBUG_BOARD_INIT, ("Putting LANai into reset\n"));
  mx_lanai_reset_on(is);
  mx_spin(20000);

  MAL_DEBUG_PRINT (MAL_DEBUG_BOARD_INIT, ("Taking board out of reset\n"));
  mx_board_reset_off(is);
  mx_spin(20000);
  mx_lx_check_parity(is, "After out-of-reset");
  /* save the eeprom strings in case we overrite them by accident */
  eeprom_strings = (volatile char *)is->lanai.sram + usable_sram_len;
  from = (void *)eeprom_strings;
  bcopy(from, is->lanai.eeprom_strings, MX_EEPROM_STRINGS_LEN);
  is->lanai.eeprom_strings[MX_EEPROM_STRINGS_LEN - 1] = 0;
  is->lanai.eeprom_strings[MX_EEPROM_STRINGS_LEN - 2] = 0;

  mx_lx_check_parity(is, "After string-spec-read");
  mx_parse_eeprom_strings(is);
  if (!is->lanai.product_code || !is->lanai.part_number || !is->mac_addr_string) {
    char *s;
    MX_WARN(("EEPROM Dump:\n"));
    for (s = is->lanai.eeprom_strings; *s; s += strlen(s) + 1) {
      MX_WARN(("\t%s\n", s));
    }
    MX_WARN(("Incomplete EEPROM, please contact help@myri.com\n"));
    return EIO;
  }
  if ((is->lanai.part_number && strcmp(is->lanai.part_number, "09-02887") == 0)
      || (is->lanai.product_code && 
	  strcmp(is->lanai.product_code, "M3S-PCIXD-4-I") == 0)) {
    MX_WARN(("PC=%s, PN=%s is not supported\n", 
	     is->lanai.product_code, is->lanai.part_number));
    return EIO;
  }
#if MX_DMA_DBG
  /* don't zero dma bitmap, since some parts are allocated very early */
  usable_sram_len -= myri_dma_pfn_max / 8;
#endif
  mx_pio_bzero((char *)(uintptr_t)is->lanai.sram, usable_sram_len);
  MAL_STBAR();
  mx_spin(20000);
  MAL_DEBUG_PRINT (MAL_DEBUG_BOARD_INIT, ("Loading MCP\n"));
  img = mx_kmalloc(usable_sram_len, MX_MZERO | MX_WAITOK);
  if (!img) {
    MX_WARN(("Could not allocate buffer to inflate mcp\n"));
    return ENOMEM;
  }
  status = mx_load_mcp(is, img, usable_sram_len, &mcp_len);
  if (status != 0) {
    MX_WARN(("%s: Could not get mcp image\n", is->is_name));
    mx_kfree(img);
    return status;
  }
  /* Copy the inflated firmware to NIC SRAM. */
  mx_pio_bcopy_write(img, (char*)(uintptr_t)is->lanai.sram, mcp_len);
  mx_kfree(img);
  mx_spin(20000);
  MAL_STBAR();
  
  /* make sure that REQ_ACK_0 is clear, otherwise lanai could start
     interrupt storm */
  if (mx_read_lanai_special_reg_u32(is, lx.ISR) & MX_LX_REQ_ACK_0)
    mx_write_lanai_isr(is, MX_LX_REQ_ACK_0);
  MAL_STBAR();

#if MX_DMA_DBG
  /* mcp needs that variable to be set at beginning of main */
  MCP_SETVAL(is, dma_dbg_lowest_addr, is->lanai.dma_dbg_lowest_addr);
  MCP_SETVAL(is, dma_dbg_pfn_max, myri_dma_pfn_max);
#endif
  MCP_SETVAL(is, lxgdb, MX_LXGDB);
  MAL_DEBUG_PRINT (MAL_DEBUG_BOARD_INIT, ("Taking LANAI out of reset\n"));
  mx_lx_int_disable(is);
  mx_lanai_reset_off(is);
  if (!is->using_msi)
    mx_lx_int_enable(is);
  MAL_STBAR();
  return 0;
}

static void
mx_lx_claim_interrupt(mx_instance_state_t *is)
{
  mx_write_lanai_isr(is, MX_LX_REQ_ACK_0);
}

static int
mx_lx_read_irq_level(mx_instance_state_t *is)
{
  return mx_read_lanai_special_reg_u32(is, lx.ISR);
}

/* map the specials and SRAM.*/

static int
mx_lx_map_board(mx_instance_state_t *is)
{
  uint32_t dma_config;

  is->lanai.special_regs = mx_map_pci_space(is, 0, MX_LX_SPECIAL_OFFSET,
					   MX_LX_SPECIAL_LEN);
  if (is->lanai.special_regs == NULL) {
    goto abort_with_nothing;
  }
  is->specials_size = MX_LX_SPECIAL_LEN;

  is->lanai.control_regs = mx_map_pci_space(is, 0, MX_LX_CONTROL_OFFSET,
					   MX_LX_CONTROL_LEN);
  if (is->lanai.control_regs == NULL) {
    goto abort_with_special_regs;
  }

  dma_config = mx_read_lanai_special_reg_u32(is, lx.DMA_CONFIG);
  is->sram_size = dma_config & MX_LX_PCI_OFFSET;

  if ((is->sram_size < 1  * 1024 * 1024) ||
      (is->sram_size > 16 * 1024 * 1024)) {
    MX_WARN (("%s: Nonsensical sram size! (0x%x)\n", is->is_name, is->sram_size));
    goto abort_with_control_regs;
  }

  is->lanai.sram = mx_map_pci_space(is, 0, 0, is->sram_size + MX_DMA_DBG * 4096);
  if (is->lanai.sram == 0) {
    goto abort_with_control_regs;
  }
#if MX_DMA_DBG
  is->lanai.dma_dbg_bitmap =(void*)(is->lanai.sram + (is->sram_size - 256 - myri_dma_pfn_max / 8));
  memset((void*)is->lanai.dma_dbg_bitmap, 0, myri_dma_pfn_max / 8);
  is->dma_dbg_bitmap_host = mx_kmalloc(myri_dma_pfn_max / 8, MX_MZERO | MX_WAITOK);
#endif


  MAL_DEBUG_PRINT (MAL_DEBUG_BOARD_INIT, 
		    ("dma_config = 0x%x\n", dma_config));

  return 0;

 abort_with_control_regs:
  mx_unmap_io_space(is, MX_LX_CONTROL_LEN, (void*)is->lanai.control_regs);
  is->lanai.control_regs = NULL;
 abort_with_special_regs:
  mx_unmap_io_space(is, MX_LX_SPECIAL_LEN, is->lanai.special_regs);
  is->lanai.special_regs = NULL;
 abort_with_nothing:
  return ENODEV;
}

static void
mx_lx_unmap_board(mx_instance_state_t *is)
{
  mx_unmap_io_space(is, is->sram_size, (void*)is->lanai.sram);
  is->lanai.sram = NULL;
  mx_unmap_io_space(is, MX_LX_CONTROL_LEN, (void*)is->lanai.control_regs);
  is->lanai.control_regs = NULL;
  mx_unmap_io_space(is, MX_LX_SPECIAL_LEN, is->lanai.special_regs);
  is->lanai.special_regs = NULL;
}


static void
mx_lx_get_freq(mx_instance_state_t *is)
{
  is->cpu_freq = MCP_GETVAL(is, clock_freq);
  if (is->cpu_freq == 0) {
    MX_WARN(("%s: Failed to read LANai clock frequency (%d)\n", 
	     is->is_name, is->cpu_freq));
    return;
  }
  
  is->pci_freq = (uint32_t)mx_read_lanai_special_reg_u16 (is, lx.PCI_CLOCK);
}

static void
mx_lx_write_kreq(mx_instance_state_t *is, mcp_kreq_t *kreq)
{
  int kreqq_index;
  
  kreqq_index = is->kreqq_submitted & (is->kreqq_max_index);
  is->kreqq_submitted++;
  mx_pio_memcpy(is->kreqq[kreqq_index].int64_array, kreq->int64_array, 
	      sizeof(mcp_kreq_t), MX_PIO_FLUSH);
  
  MCP_SETVAL(is, kreq_host_cnt, is->kreqq_submitted);
  MAL_STBAR();
}

mx_board_ops_t mx_lx_ops;

void
mx_lx_init_board_ops(void)
{
  mx_lx_ops.init		= mx_lx_init_board;
  mx_lx_ops.map			= mx_lx_map_board;
  mx_lx_ops.unmap		= mx_lx_unmap_board;
  mx_lx_ops.claim_interrupt	= mx_lx_claim_interrupt;
  mx_lx_ops.read_irq_level	= mx_lx_read_irq_level;
  mx_lx_ops.enable_interrupt	= mx_lx_int_enable;
  mx_lx_ops.disable_interrupt	= mx_lx_int_disable;
  mx_lx_ops.park		= mx_lx_park_board;
  mx_lx_ops.detect_parity_error	= mx_lx_detect_parity;
  mx_lx_ops.get_freq		= mx_lx_get_freq;
  mx_lx_ops.write_kreq		= mx_lx_write_kreq;
}
