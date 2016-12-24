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
#include "ze_mcp_defs.h"
#include "mal_byteswap.h"
#include "mcp_global.h"
#include "mcp_printf.h"
#include "lanai_Z8E_def_prefixed.h"
#include "mcp_gen_header.h"

#define LZ8E_REBOOT_PARITY_BITS (LZ8E_REBOOT_SRAM_PARITY_INT |	\
			  LZ8E_REBOOT_P0_BUFFER_PARITY_INT   |	\
			  LZ8E_REBOOT_PCIE_BUFFER_PARITY_INT |	\
			  LZ8E_REBOOT_SEND_BUFFER_PARITY_INT)


static unsigned int
mx_crc32 (const void *_ptr, unsigned int len, int big)
{
  const char *ptr = (const char *)_ptr;
  unsigned int crc = 0xffffffff;
  unsigned crc2;
  int bitnum;
  
  for (bitnum=0; bitnum < len*8;bitnum++) {
    int bit = big ? 7 - (bitnum & 7) : (bitnum & 7);
    int bitval = (ptr[bitnum/8] >> bit) & 1;
    int x32 = (crc >> 31) ^ bitval;
    crc = (crc << 1) ^ (x32 ? 0x04c11db7 : 0);
  }
  if (big)
    return ~crc;
  
  crc2 = 0;
  for (bitnum = 0;bitnum < 8; bitnum++)
    crc2 |= (0x01010101 & (crc >> bitnum)) << (7 - bitnum);
  return ~crc2;
}

#if !MX_HAS_PCIE_LINK_RESET
#define mx_pcie_link_reset(a) (-1)
#endif


/* enable interrupts */
static void
mx_lz_int_enable_always(mx_instance_state_t *is)
{
  MX_PIO_WRITE((uint32_t*)(is->lanai.sram + 0x740000),htonl(1));
  MAL_STBAR();
}

static void
mx_lz_int_enable(mx_instance_state_t *is)
{
  if (mx_is_dead(is))
    return;
  mx_lz_int_enable_always(is);
}

/* disable interrupts */
static void
mx_lz_int_disable(mx_instance_state_t *is)
{
  if (mx_is_dead(is))
    return;
  MX_PIO_WRITE((uint32_t*)(is->lanai.sram + 0x740000),htonl(2));
  MAL_STBAR();
  MAL_READBAR();
  MX_PIO_READ((volatile uint32_t*)is->lanai.sram);
}

static void
mx_lz_park_board(mx_instance_state_t *is)
{
  struct mcp_print_info *msg;
  unsigned hdr_off;
  unsigned magic;
  uint16_t cmd;
  mcp_gen_header_t *mcp_header;
  uint32_t ss_off;

  mx_read_pci_config_16(is, MX_PCI_COMMAND, &cmd);
  if (!is->bar0_low || !is->lanai.sram) {
    /* we never got far enough to try anything */
    return;
  }
  if (cmd & MX_PCI_COMMAND_MASTER) {
    hdr_off = ntohl(MX_PIO_READ((uint32_t *) (is->lanai.sram + 0x3c))) & 0xffffc;
    magic = ntohl(MX_PIO_READ((uint32_t *) (is->lanai.sram + hdr_off + 4)));
  } else {
    magic = -1;
  }
  if (myri_pcie_down == 0 &&  magic == 0x4d583130 /* MX10 */
      && is->saved_state.reason == 0) {
    struct mx_lz_handoff req;
    struct mx_page_pin p;
    uint32_t *addr;

    if (mx_alloc_dma_page(is, (char **)&addr, &p) == 0) {
      int i;
      MX_INFO(("handoff to eeprom\n"));
      req.bus_high = htonl(p.dma.high);
      req.bus_low = htonl(p.dma.low);
      req.data = 0x12345678;
      req.sram_addr = -1;
      req.mcp_len = -1;
      req.to = -1;
      req.jump_addr = -1;
      *addr = -1;
      mx_pio_memcpy((char*)is->lanai.sram + MX_LZ_HANDOFF_LOC, &req, sizeof(req), MX_PIO_FLUSH);
      for (i=0;i<100;i++) {
	if (*addr == 0x12345678)
	  break;
	mx_spin(100000);
      }
      if (*addr != 0x12345678) {
	MX_WARN(("Timeout while waiting eeprom-handoff confirmation, data=0x%x\n", *addr));
      }
      mx_write_pci_config_16(is, MX_PCI_COMMAND, cmd & ~MX_PCI_COMMAND_MASTER);
      mx_free_dma_page(is, (char **)&addr, &p);
      return;
    }
  }
  mx_write_pci_config_32(is, 0x10, is->bar0_low);
  mx_write_pci_config_32(is, 0x14, is->bar0_high);
  mx_spin(1000);
  if (myri_pcie_down_on_error == 1 &&
      (cmd & MX_PCI_COMMAND_MEMORY) &&
      mx_pcie_link_reset(is) != 0) 
    {
      /* if link reset is not available, we reboot the NIC by writing
	 a zero into the protected zone */
      is->lanai.sram[0] = 0x30;
      MAL_STBAR();
    }
  mx_spin(1000000);
  mx_write_pci_config_32(is, 0x10, is->bar0_low);
  mx_write_pci_config_32(is, 0x14, is->bar0_high);
  mx_write_pci_config_32(is, 0x18, is->bar2_low);
  mx_write_pci_config_32(is, 0x1c, is->bar2_high);
  mx_write_pci_config_16(is, 0x46, is->msi_ctrl);
  mx_write_pci_config_32(is, 0X48, is->msi_addr_low);
  mx_write_pci_config_32(is, 0X4C, is->msi_addr_high);
  mx_write_pci_config_16(is, 0x50, is->msi_data);
  mx_write_pci_config_16(is, 0X64, is->exp_devctl);


  if (is->saved_state.reason == MX_DEAD_RECOVERABLE_SRAM_PARITY_ERROR)
    return; /* leave memory-space disabled */

  mx_write_pci_config_16(is, MX_PCI_COMMAND, MX_PCI_COMMAND_MEMORY);

  hdr_off = ntohl(MX_PIO_READ((uint32_t *) (is->lanai.sram + 0x3c))) & 0xffffc;
  mcp_header = (void*)((char*)is->lanai.sram + hdr_off);
  ss_off = ntohl(MX_PIO_READ(&mcp_header->string_specs)) & 0x1ffffc;
  msg =  (void*)(is->lanai.sram + MCP_PRINT_OFF_ZE(ss_off));
  if (is->saved_state.reason != MX_DEAD_RECOVERABLE_SRAM_PARITY_ERROR &&
      MX_PIO_READ(&msg->magic) == htonl(MCP_PRINT_MAGIC)) {
    uint32_t len;
    uint32_t written;
    char *data;
    uint32_t buf_off;
    uint32_t i;

    buf_off = MX_PIO_READ(&msg->buf_off);
    buf_off = ntohl(buf_off);

    written = ntohl(MX_PIO_READ(&msg->written));
    len = written - is->ze_print_pos;
    is->ze_print_pos = written;
    if ((int)len < 0 || buf_off >= 2*1024*1024 - 128*1024)
      return;
    len = MAL_MIN(len, MCP_PRINT_BUF_SIZE);
    len = MAL_MIN(len, 2048);
    if (!len)
      return;
    data = mx_kmalloc(len + 1, MX_WAITOK);
    if (!data)
      return;
    i = 0;
    while (len > 0) {
      data[i] = MX_PIO_READ((char*)is->lanai.sram 
			    + buf_off 
			    + ((written - len + i) & (MCP_PRINT_BUF_SIZE - 1)));
      if (data[i] == 0 || data[i] == '\n' || i == len - 1) {
	data[i+1] = 0;
	MX_PRINT(("%s:Lanai: %s", is->is_name, data));
	len -= i + 1;
	i = 0;
      } else {
	i += 1;
      }
    }
    mx_kfree(data);
  }
}

static int
mx_lz_detect_parity(mx_instance_state_t *is)
{
  uint32_t reboot = -1;
  unsigned cap = mx_find_capability(is, MX_PCI_CAP_VENDOR);
  int retry = 10;

  /* retry in case we catch the NIC in the midst of a reset */
  while (!cap && retry) {
	  mx_spin(100000);
	  cap = mx_find_capability(is, MX_PCI_CAP_VENDOR);
	  retry--;
  }

  if (!cap) {
    MX_INFO(("Did device disappeard? Cannot get vendor cap\n"));
    return 0;
  }
  mx_write_pci_config_8(is, cap + 0x10, 3);
  mx_write_pci_config_32(is, cap + 0x18, 0xfffffff0);
  mx_read_pci_config_32(is, cap + 0x14, &reboot);
  MX_INFO(("REBOOT_STATUS=0x%08x\n", reboot));
  if (((reboot != -1) && (reboot & LZ8E_REBOOT_PARITY_BITS)) ||
      ((reboot & 0xfff00000) == 0x80200000)
      ) {
    char * cause = "Multiple unit";
    switch (reboot & LZ8E_REBOOT_PARITY_BITS) {
    case LZ8E_REBOOT_SEND_BUFFER_PARITY_INT:
      cause = "Send Buffer";
      break;
    case LZ8E_REBOOT_PCIE_BUFFER_PARITY_INT:
      cause = "PCIE Buffer";
      break;
    case LZ8E_REBOOT_P0_BUFFER_PARITY_INT:
      cause = "P0 Buffer";
      break;
    case LZ8E_REBOOT_SRAM_PARITY_INT:
      cause = "SRAM";
      break;
    case 0:
      cause = "PCIE FIFO";
      break;
    }
    MX_WARN(("%s: NIC has %s Parity Error\n", is->is_name, cause));
    return 1;
  }
  return 0;
}

static int
mx_lz_init_board(mx_instance_state_t *is)
{
  struct ze_hard_switch *hard_switch;
  int status;
  int mcp_len;
  void *img;
  const void *from;
  volatile char *eeprom_strings;
  unsigned crc32, mmio_crc32;
  unsigned hdr_off, magic;
  mcp_gen_header_t *mcp_header;
  unsigned load_offset;
  unsigned mcp_header_offset;
  struct ze_dump *dump;
  uint32_t dump_size = 512*1024;
  unsigned ss_off;

  mx_read_pci_config_32(is, 0x10, &is->bar0_low);
  mx_read_pci_config_32(is, 0x14, &is->bar0_high);
  mx_read_pci_config_32(is, 0x18, &is->bar2_low);
  mx_read_pci_config_32(is, 0x1c, &is->bar2_high);
  mx_read_pci_config_16(is, 0x46, &is->msi_ctrl);
  mx_read_pci_config_32(is, 0X48, &is->msi_addr_low);
  mx_read_pci_config_32(is, 0X4C, &is->msi_addr_high);
  mx_read_pci_config_16(is, 0x50, &is->msi_data);
  mx_read_pci_config_16(is, 0x64, &is->exp_devctl);

  hdr_off = ntohl(MX_PIO_READ((uint32_t *) (is->lanai.sram + 0x3c)));
  if (hdr_off == 0 || (hdr_off & 3) || hdr_off >= 1024*1024) {
    MX_INFO(("No MCP_GEN_HEADER on current firmware\n"));
    return EIO;
  }
  mcp_header = (void*)((char*)is->lanai.sram + hdr_off);
  ss_off = ntohl(MX_PIO_READ(&mcp_header->string_specs));

  /* save the eeprom strings in case we overrite them by accident */
  eeprom_strings = (volatile char *)is->lanai.sram + ss_off;
  from = (void *)eeprom_strings;
  bcopy(from, is->lanai.eeprom_strings, MX_EEPROM_STRINGS_LEN);
  is->lanai.eeprom_strings[MX_EEPROM_STRINGS_LEN - 1] = 0;
  is->lanai.eeprom_strings[MX_EEPROM_STRINGS_LEN - 2] = 0;

  mx_parse_eeprom_strings(is);

#if MAL_OS_UDRV
  if (mx_lxgdb) {
    MX_WARN(("Assuming the mcp was directly loaded into lxgdb\n"));
    return 0;
  }
#endif
  magic = MX_PIO_READ(&mcp_header->mcp_type);
  if (magic != ntohl(0x70636965) /* "PCIE" */ &&
      magic != ntohl(0x45544820) /* "ETH " */ &&
      magic != ntohl(MCP_TYPE_DFLT)) {
    char version[132];
    mx_pio_bcopy_read((void *)(uintptr_t)(is->lanai.sram + hdr_off + 4), version, 132);
    version[131] = 0;
    MX_INFO(("Did not find the std PCIE firmware, current(0x%x)=%s\n", ntohl(magic), version));
    return EIO;
  }

  
  MAL_DEBUG_PRINT (MAL_DEBUG_BOARD_INIT, ("Loading MCP\n"));
  mcp_len = 1024*1024;
  img = mx_kmalloc(mcp_len, MX_MZERO | MX_WAITOK);
  if (!img) {
    MX_WARN(("Could not allocate buffer to inflate mcp\n"));
    return ENOMEM;
  }
  status = mx_load_mcp(is, img, mcp_len, &mcp_len);
  if (status != 0) {
    MX_WARN(("%s: Could not load mcp\n", is->is_name));
    mx_kfree(img);
    return status;
  }
#if MX_DMA_DBG
  /* mcp needs those variables to be set at beginning of main */
  ((mcp_public_global_t *)((char *)img + MCP_GLOBAL_OFFSET))->dma_dbg_lowest_addr = htonl(is->lanai.dma_dbg_lowest_addr);
  ((mcp_public_global_t *)((char *)img + MCP_GLOBAL_OFFSET))->dma_dbg_pfn_max = htonl(myri_dma_pfn_max);
#endif
  mcp_header_offset = ntohl(*(uint32_t*)((char*)img + 0x3c));
  mcp_header = mcp_header_offset ? (void*)((char*)img + mcp_header_offset) : NULL;
  if (mcp_header)
    mcp_header->disable_rabbit = myri_jtag;
  load_offset = mcp_header && !myri_pcie_down ? 0x110000 : ZE_HARD_SWITCH_OFFSET;
  if (mcp_len >= ss_off - 32 * 1024 - load_offset) {
    MX_WARN(("MCP is too big: %d bytes, available for handoff = %d bytes\n",
	     mcp_len, ss_off -32 * 1024 - load_offset));
    return E2BIG;
  }
  hard_switch = (void *)(is->lanai.sram + load_offset);
  { 
    unsigned i;
    unsigned chunk;
    /* do the copy slowly to avoid padding-fifo overflows */
    for (i = 0; i < mcp_len; i+=chunk) {
      chunk = mcp_len - i;
      if (chunk > 256)
	chunk = 256;
      mx_pio_memcpy((char*)hard_switch->mcp + i, (char*)img + i, chunk, MX_PIO_32BIT_ALIGN | MX_PIO_32BYTE_FLUSH);
      (void)*(volatile uint32_t*)is->lanai.sram;
    }
  }
  MX_INFO(("%s: Loaded mcp of len %d\n", is->is_name, mcp_len));
  crc32 = mx_crc32(img, mcp_len, !is->pci_rev);
  hard_switch->crc32 = htonl(crc32);
  hard_switch->magic = htonl(ZE_HARD_SWITCH_MAGIC);
  hard_switch->size = htonl(mcp_len);
  MAL_READBAR();
  mx_pio_bcopy_read(hard_switch->mcp, img, mcp_len);
  mmio_crc32 = mx_crc32(img, mcp_len, !is->pci_rev);
  mx_kfree(img);

  if (hard_switch->crc32 != htonl(crc32) ||
      hard_switch->magic != htonl(ZE_HARD_SWITCH_MAGIC) ||
      hard_switch->size != htonl(mcp_len) ||
      mmio_crc32 != crc32) {
    MX_WARN(("Cannot switch firmware(crc=0x%x,len=%d)\n"
	     "\treread=(magic=0x%x,crc=0x%x,len=%d,crc_recomp=0x%x\n",
	     crc32, mcp_len, 
	     ntohl(hard_switch->magic), ntohl(hard_switch->crc32), 
	     ntohl(hard_switch->size), mmio_crc32));
    return EIO;
  }
  dump = (struct ze_dump*)(is->lanai.sram + ZE_DUMP_OFF(ss_off));
  dump->magic = 0;
  MAL_STBAR();
  dump->addr = htonl(ZE_DUMP_JTAG_OFF(ss_off) - dump_size);
  dump->pcie = htonl(ZE_DUMP_JTAG_OFF(ss_off) - dump_size - 32 * 1024);
  dump->size = htonl(dump_size);
  MAL_STBAR();
  dump->magic = htonl(ZE_DUMP_REQ);
  MAL_STBAR();

  if (!mcp_header || myri_pcie_down) {
    /* hard hard_switch */
    uint32_t bar0, bar1, bar2, bar3, msi_addr0, msi_addr1;
    uint16_t command, msi_control, msi_data;
    uint32_t exp_devctl;

    dump->magic = htonl(ZE_DUMP_REQ_NEXT);
    MAL_STBAR();
    /* save the important part of cfg space */
    mx_read_pci_config_32(is, 0x10, &bar0);
    mx_read_pci_config_32(is, 0x14, &bar1);
    mx_read_pci_config_32(is, 0x18, &bar2);
    mx_read_pci_config_32(is, 0x1c, &bar3);
    mx_read_pci_config_16(is, 0x4, &command);
    mx_read_pci_config_16(is, 0x46, &msi_control);
    mx_read_pci_config_32(is, 0X48, &msi_addr0);
    mx_read_pci_config_32(is, 0X4C, &msi_addr1);
    mx_read_pci_config_16(is, 0x50, &msi_data);
    mx_read_pci_config_32(is, 0x64, &exp_devctl);
    
    /* we reboot the NIC by writing a zero into the protected zone */
    is->lanai.sram[0] = 0;
    MAL_STBAR();
    /* hope two and half second is enough to boot */
    mx_spin(2500000);

    
    /* restore cfg space */
    mx_write_pci_config_32(is, 0x10, bar0);
    mx_write_pci_config_32(is, 0x14, bar1);
    mx_write_pci_config_32(is, 0x18, bar2);
    mx_write_pci_config_32(is, 0x1c, bar3);
    mx_write_pci_config_16(is, 0x4, command);
    mx_write_pci_config_16(is, 0x46, msi_control);
    mx_write_pci_config_32(is, 0x48, msi_addr0);
    mx_write_pci_config_32(is, 0x4c, msi_addr1);
    mx_write_pci_config_16(is, 0x50, msi_data);
    mx_write_pci_config_32(is, 0X64, exp_devctl);
  } else {
    int i;
    uint16_t cmd;
    struct mx_lz_handoff req;
    struct mcp_print_info *msg;

    hard_switch->magic = 0;
    MAL_STBAR();

    msg =  (void*)(is->lanai.sram + MCP_PRINT_OFF_ZE(ss_off));
    if (MX_PIO_READ(&msg->magic) == htonl(MCP_PRINT_MAGIC))
      is->ze_print_pos = ntohl(MX_PIO_READ(&msg->written));

    /* enable busmaster DMA required for soft handoff */
    mx_read_pci_config_16(is, MX_PCI_COMMAND_MASTER, &cmd);
    mx_write_pci_config_16(is, MX_PCI_COMMAND_MASTER, cmd | MX_PCI_COMMAND_MASTER);
    *(uint32_t*)is->bogus.addr = 0;
    req.bus_high = htonl(is->bogus.pin.dma.high);
    req.bus_low = htonl(is->bogus.pin.dma.low);
    req.data = htonl(0x12345678);
    req.sram_addr = htonl(hard_switch->mcp - (char*)is->lanai.sram + 8);
    req.mcp_len = htonl(mcp_len - 8);
    req.to = htonl(8);
    req.jump_addr = htonl(8);
    mx_pio_memcpy((char*)is->lanai.sram + MX_LZ_HANDOFF_LOC, &req, sizeof(req), MX_PIO_FLUSH);
    /* 5 seconds timeout */
    for (i=0;; i++) {
      if (*(uint32_t*)is->bogus.addr == htonl(0x12345678)) {
	*(uint32_t*)is->bogus.addr = 0;
	break;
      }
      if (i > 50) {
	MX_INFO(("Handoff did not work, host flag=0x%x\n", 
		 *(uint32_t*)is->bogus.addr));
	mx_lz_detect_parity(is);
	return EIO;
      }
      mx_spin(100000);
    }
  }
  MCP_SETVAL(is, z_loopback, myri_z_loopback);
  mx_lz_int_enable_always(is);
  return 0;
}

static void
mx_lz_claim_interrupt(mx_instance_state_t *is)
{
  MX_PIO_WRITE((uint32_t *) (is->lanai.sram + 0x700000), 0);
  MAL_STBAR();
}

static int
mx_lz_read_irq_level(mx_instance_state_t *is)
{
  return MX_PIO_READ((uint32_t *)is->lanai.special_regs + ZE_MMIO_IRQ / 4);
}

/* map the specials and SRAM.*/

static int
mx_lz_map_board(mx_instance_state_t *is)
{
  /* this is a temporary hack until I know how to interact with
     the mcp/bootloader */

  is->specials_size = 0;
  is->lanai.control_regs = NULL;
#ifdef MX_HAS_MAP_PCI_SPACE
  is->lanai.special_regs = mx_map_pci_space(is, 2, 0, 1024*1024);
  if (!is->lanai.special_regs)
    return ENOMEM;
#endif
  /* just pretend we have 2 MB of sram */
  if (MAL_DEBUG)
    MX_INFO(("HACK: Pretending LZ has 2MB SRAM!\n"));

  is->sram_size = 2 * 1024 * 1024;

  is->lanai.sram = mx_map_pci_space(is, 0, 0, is->board_span);
  if (is->lanai.sram == 0) {
    mx_unmap_io_space(is, 1*1024*1024, is->lanai.special_regs);
    return ENODEV;
  }
#if MX_DMA_DBG
  {
    mcp_gen_header_t * mcp_header;
    unsigned hdr_off;
    unsigned ss_off;

    hdr_off = ntohl(MX_PIO_READ((uint32_t *) (is->lanai.sram + 0x3c)));
    if (hdr_off == 0 || (hdr_off & 3) || hdr_off >= 1024*1024) {
      MX_INFO(("No MCP_GEN_HEADER on current firmware\n"));
      return EIO;
    }
    mcp_header = (void*)((char*)is->lanai.sram + hdr_off);
    ss_off = ntohl(MX_PIO_READ(&mcp_header->string_specs));
    
    is->lanai.dma_dbg_bitmap =(void*)(is->lanai.sram + ss_off - 32 * 1024 - myri_dma_pfn_max / 8);
    memset((void*)is->lanai.dma_dbg_bitmap, 0, myri_dma_pfn_max / 8);
    is->dma_dbg_bitmap_host = mx_kmalloc(myri_dma_pfn_max / 8, MX_MZERO | MX_WAITOK);
  }
#endif

  

  return 0;
}

static void
mx_lz_unmap_board(mx_instance_state_t *is)
{
  mx_unmap_io_space(is, 16*1024*1024, (void*)is->lanai.sram);
  is->lanai.sram = NULL;
#ifdef MX_HAS_MAP_PCI_SPACE
  mx_unmap_io_space(is, 1*1024*1024, is->lanai.special_regs);
  is->lanai.special_regs = NULL;
#endif
}


static void
mx_lz_get_freq(mx_instance_state_t *is)
{
  int exp_cap;
  uint16_t lnk_sta;
  
  is->cpu_freq = MCP_GETVAL(is, clock_freq);
  if (is->cpu_freq == 0) {
    MX_WARN(("%s: Failed to read LANai clock frequency (%d)\n", 
	     is->is_name, is->cpu_freq));
    return;
  }
  exp_cap = mx_find_capability(is, MX_PCI_CAP_EXP);
  if (exp_cap) {
    mx_read_pci_config_16(is, exp_cap + MX_PCI_EXP_LNK_STA, &lnk_sta);
    is->pci_freq = MX_PCI_EXP_LNK_WIDTH(lnk_sta);
  }
}

static uint32_t
mx_lz_get_rtc(mx_instance_state_t *is)
{
  return ntohl(MX_PIO_READ((uint32_t *)is->lanai.special_regs + ZE_MMIO_RTC/4));
}

static void
mx_lz_write_kreq(mx_instance_state_t *is, mcp_kreq_t *kreq)
{
  int kreqq_index;
  
  kreqq_index = is->kreqq_submitted & (is->kreqq_max_index);
  is->kreqq_submitted++;
  mx_pio_memcpy(is->kreqq[kreqq_index].int64_array, kreq->int64_array, 
	      sizeof(mcp_kreq_t), MX_PIO_FLUSH);

  MX_PIO_WRITE((uint32_t *) (is->lanai.sram + 0x700004), 0);
  MAL_STBAR();
}

mx_board_ops_t mx_lz_ops;

void
mx_lz_init_board_ops(void)
{
  mx_lz_ops.init		= mx_lz_init_board;
  mx_lz_ops.map			= mx_lz_map_board;
  mx_lz_ops.unmap		= mx_lz_unmap_board;
  mx_lz_ops.claim_interrupt	= mx_lz_claim_interrupt;
  mx_lz_ops.read_irq_level	= mx_lz_read_irq_level;
  mx_lz_ops.enable_interrupt 	= mx_lz_int_enable;
  mx_lz_ops.disable_interrupt 	= mx_lz_int_disable;
  mx_lz_ops.park		= mx_lz_park_board;
  mx_lz_ops.detect_parity_error	= mx_lz_detect_parity;
  mx_lz_ops.get_freq		= mx_lz_get_freq;
  mx_lz_ops.write_kreq		= mx_lz_write_kreq;
  mx_lz_ops.get_rtc		= mx_lz_get_rtc;
}


static uint32_t
mx_piocfg_read_u32(mx_instance_state_t *is, unsigned cap, unsigned reg)
{
  uint32_t val;
  mx_write_pci_config_8(is, cap + 0x10, 3);
  mx_write_pci_config_32(is, cap + 0x18, reg);
  mx_read_pci_config_32(is, cap + 0x14, &val);
  return val;
}


void
mx_lz_read_mac(mx_instance_state_t *is, char mac[18])
{
  int i;
  unsigned cap;
  unsigned ss_off, hdr_off;
  uint32_t ss_data[6];

  memset(mac, 0, sizeof(mac));
  cap = mx_find_capability(is, MX_PCI_CAP_VENDOR);
  if (!cap)
    return;
  
  hdr_off = mx_piocfg_read_u32(is, cap, 0x3c);
  if (hdr_off == 0 || (hdr_off & 3) || hdr_off >= 1024*1024) {
    MX_INFO(("No MCP_GEN_HEADER on current firmware\n"));
    return;
  }
  ss_off = mx_piocfg_read_u32(is, cap, hdr_off + offsetof(mcp_gen_header_t, string_specs));
  if (ss_off == 0 || (ss_off & 3) || ss_off >= 2* 1024*1024) {
    MX_INFO(("Bad string-spec off on current firmware\n"));
    return;
  }
  for (i=0; i < 6; i++)
    ss_data[i] = htonl(mx_piocfg_read_u32(is, cap, ss_off + i * 4));

  if (memcmp(ss_data, "MAC=", 4) != 0) {
    MX_INFO(("String-spec does not start with MAC\n"));
    return;
  }

  memcpy(mac, ss_data + 1, 18);
  mac[17] = 0;
}
