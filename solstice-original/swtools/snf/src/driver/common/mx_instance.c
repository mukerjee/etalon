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

#include "mx_arch.h"
#include "mx_instance.h"
#include "mx_malloc.h"
#include "mx_misc.h"
#include "kraw.h"
#include "mx_pio.h"
#include "mcp_config.h"
#include "mal_stbar.h"
#include "mcp_global.h"

#include "mx_ether_common.h"


mx_instance_state_t **mx_instances;
uint32_t myri_intr_coal_delay = MCP_INTR_COAL_DELAY;

uint32_t myri_max_instance = MX_MAX_INSTANCE_DEFAULT;
uint32_t mx_num_instances;
uint32_t myri_mx_max_nodes = MCP_NODES_CNT;
uint32_t myri_max_endpoints = MCP_ENDPOINTS_CNT;
uint32_t myri_recvq_vpage_cnt = MCP_RECVQ_VPAGE_CNT;
uint32_t myri_eventq_vpage_cnt = MCP_EVENTQ_VPAGE_CNT;
uint32_t myri_mx_max_send_handles = MCP_SEND_HANDLES_CNT;
uint32_t mx_max_push_handles = MCP_PUSH_HANDLES_CNT;
uint32_t myri_mx_max_rdma_windows = MCP_RDMA_WINDOWS_CNT;
uint32_t mx_cacheline_size = 0;
uint32_t myri_override_e_to_f = 0;
uint32_t myri_z_loopback = 0;
uint32_t myri_pcie_down = MX_RDMA_FC;
uint32_t myri_parity_recovery = 1;
uint32_t myri_recover_from_all_errors = 0;
uint32_t myri_mx_max_host_queries = 0;
uint32_t myri_pcie_down_on_error = 1;

char mx_default_hostname[MYRI_MAX_STR_LEN];
uint32_t myri_mx_max_macs;

#if MX_ETHER_MODE
uint32_t myri_ethernet_bitmap = -1;
#else
uint32_t myri_ethernet_bitmap = 0;
#endif
int mx_has_xm = 0;
char *myri_mac = NULL;

#if MAL_OS_LINUX && MX_DMA_DBG
uint32_t myri_dma_pfn_max = 8216*1024/4;/* 8GB with 4k pages == 256KB of SRAM */
#elif MX_DMA_DBG
/* only supported under linux, for all OSes, 0 will actually disables the mcp checking */
uint32_t myri_dma_pfn_max = 0;
#endif
uint32_t myri_force = 0;
uint32_t myri_jtag = 1; /* bit0: disable_rabbit, bit1: disable_scan */

/* force PIO read in interrupt handler to wait for descriptor DMA */
uint32_t myri_irq_sync = 0; 

uint32_t myri_ether_pause = 0;


static int
mx_select_board_type(mx_instance_state_t *is, int vendor_id, 
		     int device_id, int pci_revision)
{
  int rc = ENODEV;

  if (vendor_id != MX_PCI_VENDOR_MYRICOM)
    goto error;

  switch (device_id) {
  case MX_PCI_DEVICE_MYRINET:
    if (myri_override_e_to_f && pci_revision == 5)
      pci_revision = 6;

    switch (pci_revision) {
    case 4:
      /* fallthrough */
    case 6:
      is->board_type = MAL_BOARD_TYPE_D;
      rc = 0;
      break;  

    case 5:
      is->board_type = MAL_BOARD_TYPE_E;
      rc = 0;
      break;  

    default:
      break;
    }
    break;

  case MX_PCI_DEVICE_Z4E:
    /* fallthrough */
  case MX_PCI_DEVICE_Z8E:
  case MX_PCI_DEVICE_Z8E_9:
      if (myri_ethernet_bitmap & (1 << is->id))
	is->board_type = pci_revision ? MAL_BOARD_TYPE_ZOES : MAL_BOARD_TYPE_ZOE;
      else
	is->board_type = pci_revision ? MAL_BOARD_TYPE_ZOMS : MAL_BOARD_TYPE_ZOM;
      rc = 0;
      break;  

  default:
    break;  
  }
  if (rc)
    goto error;
  is->num_ports = MAL_NUM_PORTS(is->board_type);

  /* setup function pointers for routines which vary by board type */
  switch (is->board_type) {
  case MAL_BOARD_TYPE_D:	
    /* fallthrough */
  case MAL_BOARD_TYPE_E:
    is->board_ops = mx_lx_ops;
    break;
  case MAL_BOARD_TYPE_ZOM:
  case MAL_BOARD_TYPE_ZOE:
  case MAL_BOARD_TYPE_ZOMS:
  case MAL_BOARD_TYPE_ZOES:
    is->board_ops = mx_lz_ops;
    break;
  default:
    break;
  }    

  /* refuse to load if user has compiled out support
     for a type of board */
  if (is->board_ops.map == NULL) {
    rc = ENODEV;
    goto error;
  }

  return 0;

 error:
  MX_WARN(("Unknown/Unsupported board type 0x%x:0x%x rev %d\n",
	   vendor_id, device_id, pci_revision));
  return rc;
}

    
static void
mx_restore_pci_cap(mx_instance_state_t *is)
{
  int status;

  if (is->pci_cap.command_tweaked) {
    is->pci_cap.command_tweaked = 0;
    status = mx_write_pci_config_16(is, is->pci_cap.command_offset,
				    is->pci_cap.command_orig_val);
    if (status) {
      MX_WARN(("Unable to restore PCI-X capability register to oringal value\n"));
    }
    
  }
}

int 
mx_find_capability(mx_instance_state_t *is, unsigned cap_id)
{
  uint8_t cap = 0;
  uint8_t id = 0;
  uint16_t status;
  if (mx_read_pci_config_16(is, MX_PCI_STATUS, &status) != 0
      || !MX_PCI_STATUS_CAPABILITIES_LIST(status)) {
    MX_WARN(("%s:mx_find_capability:No cap list!!!\n", is->is_name));
    return 0;
  }
  if (mx_read_pci_config_8(is, offsetof(mx_pci_config_t, Cap_List), &cap) != 0) {
    MX_WARN(("%s:mx_find_capability:config-space failure\n", is->is_name));
    return 0;
  }
  while (cap) {
    if (cap == 0xff || cap < 0x40) {
      MX_WARN(("%s:invalid cap list, found cap-ptr = 0x%x\n", is->is_name, cap));
      return 0;
    }
    cap &= 0xfc;
    if (mx_read_pci_config_8(is, cap, &id)) {
      MX_WARN(("%s:mx_find_capability:config-space failure\n", is->is_name));
      return 0;
    }
    if (id == cap_id)
      return cap;
    if (mx_read_pci_config_8(is, cap + 1, &cap) != 0) {
      MX_WARN(("%s:mx_find_capability:config-space failure\n", is->is_name));
      return 0;
    }
  }
  return cap;
}

/* check to see if we're using MSIs */

static int
mx_check_for_msi(mx_instance_state_t *is)
{
  int status;
  uint16_t message_control;
  uint8_t msi_cap;

  msi_cap = mx_find_capability(is, MX_PCI_CAP_MESSAGE_SIGNALLED_INTERRUPTS);
  if (msi_cap) 
    {
      /* Read the message_control field from the capability struct,
	 which has some bits we need. */
      
      MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT, ("Reading Message_Control.\n"));
      status = mx_read_pci_config_16
	(is,
	 msi_cap + offsetof(mx_pci_capability_t, msi.Message_Control),
	 &message_control);
      if (status != 0) {
	MX_WARN(("Could not read msi.Message_Control\n"));
	return status;
      }
      
      /* Determine if the OS enabled MSI. */
      
      MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT, ("Checking MSI enable.\n"));
      if (MX_PCI_CAP_MESSAGE_CONTROL_MSI_ENABLE (message_control)) {
	
	/* The OS told the card to use MSI. */
	
	is->using_msi = 1;
      } else {
	uint16_t pci_cmd;
	/* The OS told the card not to use MSI. */
	
	MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT, ("OS disabled MSI\n"));
	mx_read_pci_config_16(is, MX_PCI_COMMAND, &pci_cmd);
	if (pci_cmd & MX_PCI_COMMAND_INTX_DISABLE) {
	  MX_WARN(("%s: PCI Cmd INTX_DISABLE was set, clear\n", is->is_name));
	  mx_write_pci_config_16(is, MX_PCI_COMMAND, 
				 pci_cmd & ~MX_PCI_COMMAND_INTX_DISABLE);
	}

      }
	    
    }
  return 0;
}

  
static int
mx_check_for_pcix_rbc(mx_instance_state_t *is)
{
  int status;
  uint8_t pcix_cap;
  uint16_t command, max_read_byte_cnt;

  pcix_cap = mx_find_capability(is, MX_PCI_CAP_PCI_X);

  if (pcix_cap) {
      MAL_DEBUG_PRINT(MAL_DEBUG_BOARD_INIT, ("Found a PCI-X capability\n"));
      
      /* Read the PCI-X command value */
      
      status = mx_read_pci_config_16
	(is,
	 pcix_cap + offsetof(mx_pci_capability_t, pci_x.Command),
	 &command);
      if (status != 0) {
	MX_WARN(("Could not read pci_x.Command\n"));
	return status;
      }
              
      /* Determine the maximum memory read byte count set by
	 the BIOS or OS. */

      max_read_byte_cnt = command 
	& MX_PCI_CAP_PCI_X_COMMAND_MAX_MEM_READ_BYTE_CNT_MASK	;
      
      /* Whine if setting is not optimial */
      if (max_read_byte_cnt < 
	  MX_PCI_CAP_PCI_X_COMMAND_MAX_MEM_READ_BYTE_CNT_4096) {
	MX_INFO(("%s: BIOS or OS set PCI-X max memory read byte count < 4KB\n", is->is_name));
      }

      /* Update the value if it is unacceptable (<2048).  We really
	 would like to use 4096 byte reads, but that is known not to
	 be reliable with some chipsets.  Using  2048 instead only
	 reduces bidirectional bandwidth  about 5% and does not
	 effect unidirectional  performance, according to our
	 tests. 
      */
      
      if (MX_PCI_X_FORCE_LARGE_READ &&
	  (max_read_byte_cnt < 	  	
	   MX_PCI_CAP_PCI_X_COMMAND_MAX_MEM_READ_BYTE_CNT_2048)) {

	is->pci_cap.command_tweaked = 1;
	is->pci_cap.command_offset = 
	  pcix_cap + offsetof(mx_pci_capability_t, pci_x.Command);
	is->pci_cap.command_orig_val = command;
	
	command = (command ^ max_read_byte_cnt ^ 
		   MX_PCI_CAP_PCI_X_COMMAND_MAX_MEM_READ_BYTE_CNT_2048);
	
	MX_WARN(("%s: Forcing PCI-X max_mem_read_byte_cnt to 2KB\n", is->is_name));
	
	/* Write back the PCI-X command value */
	
	status = mx_write_pci_config_16(is, is->pci_cap.command_offset,
					command);
	if (status != 0) {	
	  MX_WARN(("%s: Could not write pci_x.Command\n", is->is_name));
	  return status;
	}
      }
  }
    
  return 0;
}


static int
mx_disable_pci_config_command_bit(mx_instance_state_t * is,  uint16_t value)
{
  uint16_t command;
  int status;

  status = mx_read_pci_config_16(is, offsetof(mx_pci_config_t, Command),
				 &command);
  if (status) {
    MX_PRINT(("Could not read PCI command register.\n"));
    return (status);
  }
  command &= ~(value);
  
  status = mx_write_pci_config_16(is, offsetof(mx_pci_config_t, Command),
				  command);
  if (status) {
    MX_PRINT(("Could not write PCI command register.\n"));
    return (status);
  }

  /* Pause for at least 10ms */
  mx_spin (15000);
  status = mx_read_pci_config_16(is, offsetof(mx_pci_config_t, Command),
			       &command);
  return status;
}

static int
mx_enable_pci_config_command_bit(mx_instance_state_t *is,  uint16_t value)
{
  uint16_t command;
  int status;

  status = mx_read_pci_config_16(is, offsetof(mx_pci_config_t, Command),
				 &command);
  if (status) {
    MX_WARN(("%s: Could not read PCI command register.\n",
	      is->is_name));
    return (status);
  }
  command |= value;
  
  status = mx_write_pci_config_16(is, offsetof(mx_pci_config_t, Command),
				  command);
  if (status) {
    MX_WARN(("%s: Could not write PCI command register.\n",
	     is->is_name));
    return (status);
  }

  /* Pause for at least 10ms */
  mx_spin (15000);

  status = mx_read_pci_config_16(is, offsetof(mx_pci_config_t, Command),
				 &command);
  if (status) {
    MX_WARN(("%s: Could not read PCI command register.\n",
	      is->is_name));
    return (status);
  }

  if ((command & value) != value) {
    MX_WARN(("%s: Couldn't set pci config command bit 0x%x\n",
	      is->is_name, value));
    status = EIO;
  }

  return status;
}


void
mx_lanai_print(mx_instance_state_t *is, int idx)
{
  char *c;
  int newline = 0;
  uint32_t *mcp_print_limit;
  unsigned long flags;

  flags = 0;  /* useless initialization to pacify -Wunused */

  mx_spin_lock_irqsave(&mx_lanai_print_spinlock, flags);
  c = is->mcp_print_buffer;
  if (is->mcp_print_len && idx < is->mcp_print_len) {
    do {
      if (newline) {
	*c = '\0';
	c = is->mcp_print_buffer;
	MX_PRINT(("LANai[%d]: %s", is->id, c));
      }
      *c = ((char *)is->lanai.sram + is->mcp_print_addr)[is->mcp_print_idx];
      newline = (*c == '\n');
      c++;
      is->mcp_print_idx++;
      if (is->mcp_print_idx >= is->mcp_print_len)
	is->mcp_print_idx = 0;
    } while (idx != is->mcp_print_idx);
    *c = '\0';
    MX_PRINT(("LANai[%d]: %s", is->id, is->mcp_print_buffer));
    is->mcp_print_idx = idx;
    mcp_print_limit = (uint32_t*) MCP_GETPTR(is, print_buffer_limit);
    if (idx == 0) {
      *mcp_print_limit = htonl(is->mcp_print_len - 1);
    }
    else {
      *mcp_print_limit = htonl(idx - 1);
    }
    MAL_STBAR();
  } else if (is->mcp_print_len && idx >= is->mcp_print_len) {
    MX_WARN(("%s: print interrupt with invalid index %d, max %d\n",
	     is->is_name, idx, is->mcp_print_len - 1));
  }
  mx_spin_unlock_irqrestore(&mx_lanai_print_spinlock, flags);
}

static uint32_t
mx_strtou32(const char* str)
{
  uint32_t ui;
  const char* p;

  ui = 0;
  for (p = str; *p >= '0' && *p <= '9'; ++p) {
    ui *= 10;
    ui += (*p - '0');
  }
  return ui;
}

/*
 * The eeprom strings on the lanaiX have the format
 * SN=x\0
 * MAC=x:x:x:x:x:x\0
 * PT:ddd mmm xx xx:xx:xx xx\0
 * PV:ddd mmm xx xx:xx:xx xx\0
 */
void
mx_parse_eeprom_strings(mx_instance_state_t *is)
{
#define MX__NEXT_STRING(p) while(*ptr++)
  char *ptr = is->lanai.eeprom_strings;
  int i, hv, lv;

  while (*ptr != '\0') {
    if (memcmp(ptr, "SN=", 3) == 0) {
      is->lanai.serial = mx_strtou32(ptr + 3);
    }
    else if (memcmp(ptr, "MAC=", 4) == 0) {
      ptr+=4;
      is->mac_addr_string = ptr;
      for (i = 0; i < 6; i++) {
	if ((ptr + 2) - is->lanai.eeprom_strings >=  MX_EEPROM_STRINGS_LEN)
	  goto abort;

	if (*(ptr+1) == ':') {
	  hv = 0;
	  lv = mx_digit(*ptr); ptr++;
	}
	else {
	  hv = mx_digit(*ptr); ptr++;
	  lv = mx_digit(*ptr); ptr++;
	}
	is->mac_addr[i] = (hv << 4) | lv;
	ptr++;
      }
      ptr--;
    }
    else if (memcmp(ptr, "PC=", 3) == 0) {
      is->lanai.product_code = ptr + 3;
    }
    else if (memcmp(ptr, "PN=", 3) == 0) {
      is->lanai.part_number = ptr + 3;
    }
    else if (ptr[0] < 32 || ptr[0] >= 127) {
      MX_WARN(("skipping invalid eeprom string %s\n", ptr));
    }
    MX__NEXT_STRING(ptr);
  }
  MX_INFO(("%s: MAC address = %s\n", is->is_name, is->mac_addr_string));
  return;
 abort:
  MX_WARN(("Failed to parse eeprom strings\n"));
  MX_WARN(("strings = %p, ptr = %p\n",is->lanai.eeprom_strings, ptr));
}

static void
mx_freeze_board(mx_instance_state_t *is)
{
  /* Leave the CPU in reset, just in case firmware
     has gone nuts */
  is->flags |= MX_IS_DEAD;
  is->board_ops.disable_interrupt(is);
  is->board_ops.park(is);
  if (is->lanai.eeprom_strings) {
    mx_kfree(is->lanai.eeprom_strings);
    is->lanai.eeprom_strings = NULL;
  }
  if (is->dma_dbg_bitmap_host)
    mx_kfree(is->dma_dbg_bitmap_host);
  is->dma_dbg_bitmap_host = NULL;
}

void
mx_parse_mcp_error(mx_instance_state_t *is)
{
  uint32_t mcp_status, idx;
  uint16_t pci_status = -1;

  mx_read_pci_config_16(is, offsetof(mx_pci_config_t, Status), &pci_status);
  MX_WARN(("PCI-status=0x%04x\n", pci_status));
#ifdef MX_HAS_BRIDGE_PCI_SEC_STATUS
  pci_status = mx_bridge_pci_sec_status(is);
  MX_WARN(("BRIDGE PCI-sec-status=0x%04x\n", pci_status));
#endif
  
  if (MAL_IS_ZE_BOARD(is->board_type) && mx_is_dead(is)) {
    MX_WARN(("Cannot parse error under dead ze\n"));
    return;
  }
  /* look at the MCP status */
  mcp_status = MCP_GETVAL(is, mcp_status);

  switch (mcp_status) {
  case MCP_STATUS_ERROR:
    MX_WARN(("A fatal error occured in the firmware !\n"));
    break;
    
  case MCP_STATUS_LOAD:
    MX_WARN(("The firmware died before the initialization phase "
	      "(status is MCP_STATUS_LOAD) !\n"));
    break;
    
  case MCP_STATUS_INIT:
    MX_WARN(("The firmware died during the initialization phase "
	      "(status is MCP_STATUS_INIT) !\n"));
    break;
    
  case MCP_STATUS_RUN:
    MX_WARN(("The firmware died after the initialization phase "
	      "(status is MCP_STATUS_RUN) !\n"));
    break;
    
  case MCP_STATUS_PARITY:
    MX_WARN(("The firmware stopped after a SRAM parity error "
	      "(status is MCP_STATUS_PARITY) !\n"));
    break;
    
  default:
    MX_WARN(("The firmware died for an unknown reason (status is 0x%x).\n", 
	      mcp_status));
  }

  /* push out any pending LANai prints */
  idx = MCP_GETVAL(is, print_buffer_pos);
  if (idx != is->mcp_print_idx) {
    MX_WARN(("Dumping LANai printf buffer on instance %d:\n", is->id));
    mx_lanai_print(is, idx);
  }
}

static int
mx_init_board(mx_instance_state_t *is, int recovering, uint32_t endpoint_bitmap)
{
  int ms, status;
  uint32_t mcp_status, rx_detect, lane_select;
  uint32_t bridge_id = 0;

  /* disable busmaster DMA while we fiddle with the board */
  status = mx_disable_pci_config_command_bit(is, MX_PCI_COMMAND_MASTER);
  if (status) {
    MX_WARN(("could not disable PCI busmaster DMA\n"));
    return(status);
  }

#ifdef MX_HAS_BRIDGE_ID
    bridge_id = mx_bridge_id(is);
#endif
  if (!recovering) {
    MX_INFO(("%s: device %x, rev %d, %d ports and %d bytes of SRAM available\n",
	      is->is_name, is->pci_devid, is->pci_rev, is->num_ports,
	      is->sram_size - MX_EEPROM_STRINGS_LEN));
    if (bridge_id)
      MX_INFO(("%s: Bridge is %04x:%04x\n", is->is_name, bridge_id & 0xffff, bridge_id >> 16));
  }

  status = is->board_ops.init(is);
  if (status) {
    goto abort;
  }
  ms = 0;
  do {
    mx_sleep(&is->init_sync, MX_SMALL_WAIT, MX_SLEEP_NOINTR);
    ms += MX_SMALL_WAIT;
    mcp_status = MCP_GETVAL(is, mcp_status);
  } while (mcp_status == MCP_STATUS_LOAD && ms <= MCP_INIT_TIMEOUT);
  
  if (mcp_status != MCP_STATUS_INIT) {
    MX_WARN(("Timed out waiting for MCP to INIT, mcp_status = 0x%x\n", 
	      mcp_status));
    mx_parse_mcp_error(is);
    status = ENXIO;
    goto abort;
  }

  /* enable busmaster DMA now that mcp is loaded */
  status = mx_enable_pci_config_command_bit(is, MX_PCI_COMMAND_MASTER);
  if (status) {
    MX_WARN(("could not enable PCI busmater DMA\n"));
    goto abort;
  }


  MCP_SETVAL(is, mac_high32, (is->mac_addr[0] << 24) | (is->mac_addr[1] << 16) 
	     | (is->mac_addr[2] << 8) | is->mac_addr[3]);
  MCP_SETVAL(is, mac_low16, (is->mac_addr[4] << 8) | is->mac_addr[5]);
  MCP_SETVAL(is, mac_high16, (is->mac_addr[0] << 8) | is->mac_addr[1]);
  MCP_SETVAL(is, mac_low32, (is->mac_addr[2] << 24) | (is->mac_addr[3] << 16) 
	     | (is->mac_addr[4] << 8) | is->mac_addr[5]);

  MCP_SETVAL(is, endpt_recovery, endpoint_bitmap);
  MCP_SETVAL(is, nodes_cnt, myri_mx_max_nodes);
  MCP_SETVAL(is, endpoints_cnt, myri_max_endpoints);
  MCP_SETVAL(is, recvq_vpage_cnt, myri_recvq_vpage_cnt);
  MCP_SETVAL(is, eventq_vpage_cnt, myri_eventq_vpage_cnt);
  MCP_SETVAL(is, send_handles_cnt, myri_mx_max_send_handles);
  MCP_SETVAL(is, push_handles_cnt, mx_max_push_handles);
  MCP_SETVAL(is, rdma_windows_cnt, myri_mx_max_rdma_windows);
  MCP_SETVAL(is, random_seed, mx_rand());
  MCP_SETVAL(is, raw_recv_enabled, 0);
  MCP_SETVAL(is, intr_coal_delay, myri_intr_coal_delay);

  MCP_SETVAL(is, intr_dma.low, is->intr.pin.dma.low);
  MCP_SETVAL(is, intr_dma.high, is->intr.pin.dma.high);
  MCP_SETVAL(is, endpt_desc_dma.low, is->endpt_desc.pin.dma.low);
  MCP_SETVAL(is, endpt_desc_dma.high, is->endpt_desc.pin.dma.high);
  MCP_SETVAL(is, tx_done_vpage.low, is->tx_done.pin.dma.low);
  MCP_SETVAL(is, tx_done_vpage.high, is->tx_done.pin.dma.high);
  MCP_SETVAL(is, sysbox_vpage.low, is->sysbox.pin.dma.low);
  MCP_SETVAL(is, sysbox_vpage.high, is->sysbox.pin.dma.high);
  MCP_SETVAL(is, counters_vpage.low, is->counters.pin.dma.low);
  MCP_SETVAL(is, counters_vpage.high, is->counters.pin.dma.high);
  MCP_SETVAL(is, bogus_vpage.low, is->bogus.pin.dma.low);
  MCP_SETVAL(is, bogus_vpage.high, is->bogus.pin.dma.high);
  MCP_SETVAL(is, pcie_down_on_error, myri_pcie_down_on_error);
  MCP_SETVAL(is, host_page_size, MX_PAGE_SIZE);
  MCP_SETVAL(is, ether_pause, myri_ether_pause);
  
  /* printf support */
  is->mcp_print_len = MCP_PRINT_BUFFER_SIZE;
  is->mcp_print_addr = MCP_GETVAL(is, print_buffer_addr);
  if (recovering)
    mx_kfree(is->mcp_print_buffer);
  is->mcp_print_buffer = mx_kmalloc(is->mcp_print_len + 1, MX_MZERO | MX_WAITOK);
  if (is->mcp_print_buffer == NULL) {
    MX_WARN(("Can't allocate MCP host printf buffer\n"));
    is->mcp_print_len = 0;
  }

  if (is->mcp_print_len != 0) {
    uint32_t *mcp_print_limit;
    mcp_print_limit = (uint32_t*) MCP_GETPTR(is, print_buffer_limit);
    *mcp_print_limit = htonl(is->mcp_print_len - 1);
    MAL_STBAR();
  }

  is->intr.slot = 0;
  is->intr.seqnum = 0;

  /* let the MCP know we've finished setting things up, and that it
     may now send us an interrupt */

  MCP_SETVAL(is, params_ready, 1);

  MAL_DEBUG_PRINT (MAL_DEBUG_BOARD_INIT, ("run, baby, run!\n"));

  do {
    mx_sleep(&is->init_sync, MX_SMALL_WAIT, MX_SLEEP_NOINTR);
    ms += MX_SMALL_WAIT;
    mcp_status = MCP_GETVAL(is, mcp_status);
  } while (mcp_status == MCP_STATUS_INIT && ms <= MCP_INIT_TIMEOUT);

  if (mcp_status != MCP_STATUS_RUN) {
    MX_WARN(("Timed out waiting for MCP to RUN, mcp_status = 0x%x\n", 
             mcp_status));
    mx_parse_mcp_error(is);
    status = ENXIO;
    goto abort;
  }

  is->kreqq = (mcp_kreq_t *)(is->lanai.sram + MCP_GETVAL(is, kreqq_offset));
  is->kreqq_submitted = 0;
  is->kreqq_max_index = MCP_KREQQ_CNT - 1;

  is->counters.mcp = (uint32_t *)(is->lanai.sram + MCP_GETVAL(is, counters_offset));

  is->mmu.mcp_hash = (uint32_t *)(is->lanai.sram + MCP_GETVAL(is, mmu_hash_offset));
  
  rx_detect = MCP_GETVAL(is, pcie_rx_detect);
  lane_select = MCP_GETVAL(is, pcie_lane_select);
  if (rx_detect & ~lane_select) {
    if (rx_detect != 0x01 && rx_detect != 0x03
	&& rx_detect != 0x0f && rx_detect != 0xff) {
      /* rx_detect not a power of two */
      MX_WARN(("Bad pcie-slot lanes: pcie lanes detect/select=%04x/%04x\n",
	       rx_detect, lane_select));
    } else if ((bridge_id & 0xffff) == 0x1166) {
      MX_WARN(("Serverworks/Broadcom chipset bug: pcie lanes detect/select=%04x/%04x\n",
	       rx_detect, lane_select));
      MX_WARN(("Please upgrade NIC eeprom to a special \"ht2100\" version\n"));
    } else {
      MX_WARN(("Bad PCI-E x1/x2/x4 negotiation: pcie lanes detect/select=%04x/%04x\n",
	       rx_detect, lane_select));
      MX_WARN(("Please upgrade NIC eeprom\n"));
    }
    if (!myri_force) {
      status = EIO;
      goto abort;
    }
  }
  is->snf.rx_doorbell_be64 =
    (uint64_t *)(is->lanai.sram + MCP_GETVAL(is, snf_rx_doorbell_off));


  return 0;

 abort:
  mx_freeze_board(is);

  if (is->mcp_print_len != 0) {
    is->mcp_print_len = 0;
    mx_kfree(is->mcp_print_buffer);
    is->mcp_print_buffer = NULL;
  }

  return(status);
}


static int
mx_alloc_intr(mx_instance_state_t *is)
{
  /* This can prevent a bad memory access in the interrupt handler
     with a shared irq */
  is->intr.ring = 0;
  return mx_alloc_zeroed_dma_page(is, (void *)&is->intr.ring, &is->intr.pin);
}

static void
mx_free_intr(mx_instance_state_t *is)
{
  mx_free_dma_page(is, (void *)&is->intr.ring, &is->intr.pin);
  is->intr.ring = 0;
}

int  
mx_instance_init (mx_instance_state_t *is, int32_t unit)
{
  int status;
  uint16_t vendor;
  uint16_t device;
  uint8_t cacheline_size;

  status = mx_enable_pci_config_command_bit(is, MX_PCI_COMMAND_MEMORY);
  if (status) {
    MX_WARN(("Unable to enable PCI memory space access for board %d\n",
	     unit));
    goto abort_with_nothing;
  }
    
  is->id = unit;

  status = mx_read_pci_config_16(is, offsetof(mx_pci_config_t, 
					      Vendor_ID), &vendor);
  if (status) {
    MX_WARN(("Could not determine board vendor id\n"));
    return ENXIO;
  }
  
  status = mx_read_pci_config_16(is, offsetof(mx_pci_config_t, 
					      Device_ID), &device);
  if (status) {
    MX_WARN(("Could not determine board device id\n"));
    return ENXIO;
  }  
  
  status = mx_read_pci_config_8(is, offsetof(mx_pci_config_t, 
					     Revision_ID), &is->pci_rev);
  if (status) {
    MX_WARN(("Could not determine board revision id\n"));
    return ENXIO;
  }

  status = mx_read_pci_config_16(is, offsetof(mx_pci_config_t, 
					     Device_ID), &is->pci_devid);
  if (status) {
    MX_WARN(("Could not determine board device id\n"));
    return ENXIO;
  }
  
  status = mx_read_pci_config_8(is, offsetof(mx_pci_config_t, 
					      Cache_Line_Size), 
				 &cacheline_size);
  if (status) {
    MX_WARN(("Could not determine board cache line size\n"));
    return ENXIO;
  }
  
  /* set the number of ports (used for managing routes) */

  status = mx_select_board_type(is, vendor, device, is->pci_rev);
  if (status != 0)
    return ENXIO;

  /* cachline size is reported in units of "dwords" (4 bytes), but we
     want it in bytes */
  if (mx_cacheline_size == 0)
    mx_cacheline_size = 4 * (uint32_t)cacheline_size;
  
  if (mx_cacheline_size != (uint32_t)sizeof(uint32_t) * (uint32_t)cacheline_size)
    MX_WARN(("Different boards are marked with different cacheline sizes? old = %d, new = %d\n",
	     mx_cacheline_size, 4 * (uint32_t)cacheline_size));

  status = is->board_ops.map(is);
  if (status) {
    MX_WARN(("%s: Could not map the board\n", is->is_name));
    goto abort_with_nothing;
  }


  /* SNF uses the following extra 'N' endpoints for max_endpoints,
   *
   * N for TX endpoints, these are actual mcp-level endpoints
   * N for RX handles
   * N for RX ring handles
   * 1 for RX kagent (if used)
   */
  is->es_count = 3*myri_max_endpoints + 1;

  /* malloc the array of endpoints */
  is->es = mx_kmalloc 
    (sizeof(is->es[0]) * is->es_count, MX_MZERO);

  if (is->es == 0) {
    status = ENOMEM;
    MX_WARN(("Could not allocate the endpoints\n"));
    goto abort_with_host_query;
  }
    
  mx_sync_init(&is->logging.sync, is, -1, "logging sync");
  mx_sync_init(&is->dmabench.wait_sync, is, -1, "dmabench wait sync");
  mx_spin_lock_init(&is->kreqq_spinlock, is, -1, "kreqq spinlock");

  mx_atomic_set(&is->ref_count, 0);

  
  status = mx_check_for_msi(is);
  if (status) {
    MX_WARN(("Could not check for MSI support\n"));
    goto abort_with_malloc;
  }
  status = mx_check_for_pcix_rbc(is);
  if (status) {
    MX_WARN(("Could not adjust PCIX RBC\n"));
    goto abort_with_malloc;
  }

  status = mx_alloc_intr(is);
  if (status) {
    MX_WARN(("Could not allocate interrupt DMA page\n"));
    goto abort_with_rbc;
  }

  status = mx_alloc_zeroed_dma_page(is, &is->endpt_desc.addr, &is->endpt_desc.pin);
  if (status) {
    MX_WARN(("Could not allocate the endpoint desc DMA page\n"));
    goto abort_with_intr_ring;
  }

  status = mx_alloc_dma_page_relax_order(is, &is->tx_done.addr, &is->tx_done.pin);
  if (status) {
    MX_WARN(("Could not allocate the TX completion DMA page\n"));
    goto abort_with_endpt_desc;
  }

  status = mx_alloc_dma_page(is, &is->sysbox.addr, &is->sysbox.pin);
  if (status) {
    MX_WARN(("Could not allocate the sysbox DMA page\n"));
    goto abort_with_tx_done;
  }
  
  status = mx_alloc_dma_page_relax_order(is, &is->counters.addr, &is->counters.pin);
  if (status) {
    MX_WARN(("Could not allocate the counters DMA page\n"));
    goto abort_with_sysbox;
  }
  
  status = mx_alloc_dma_page_relax_order(is, &is->bogus.addr, &is->bogus.pin);
  if (status) {
    MX_WARN(("Could not allocate the bogus DMA page\n"));
    goto abort_with_counters;
  }
  

  is->mmu.host_table = mx_kmalloc(MX_MMU_CNT * sizeof(mx_mmu_t), 
				  MX_WAITOK | MX_MZERO);
  if (is->mmu.host_table == 0) {
    status = ENOMEM;
    MX_WARN(("Could not allocate the MMU table\n"));
    goto abort_with_routes;
  }

  mx_sync_init(&is->sync, is, -1, "is->sync");
  mx_sync_init(&is->init_sync, is, -1, "is->init_sync");
  mx_spin_lock_init(&is->cmd.spinlock, is, -1, "cmd spinlock");
  is->snf.is = is;
  mx_sync_init(&is->snf.sync, is, -1, "is->snf.sync");
  mx_sync_init(&is->snf.kagent.sync, is, -1, "is->snf.kagent.sync");
  mx_spin_lock_init(&is->snf.rx_intr_lock, is, -1, "SNF intr spinlock");

  is->lanai.eeprom_strings = mx_kmalloc(MX_EEPROM_STRINGS_LEN, MX_NOWAIT);
  if (!is->lanai.eeprom_strings) {
    MX_WARN(("unable to allocate buffer to save eeprom strings\n"));
    goto abort_with_sync;
  }

  mx_mmu_init(is);

  status = mx_init_board(is, 0, 0);
  if (status) {
    MX_WARN(("Could not init the board\n"));
    /* note that the eeprom strings are cleaned up inside
       of mx_init_board() */
    goto abort_with_sync;  
  }


#if MYRI_ENABLE_PTP
  status = myri_ptp_alloc(is);
  if (status) {
    MX_WARN(("Could not init PTP\n"));
    goto abort_with_sync;
  }
#endif

  is->board_ops.get_freq(is);
  mx_mutex_enter(&mx_global_mutex);
  mx_mutex_enter(&is->sync);
  mx_instances[is->id] = is;
  mx_num_instances += 1;
  mx_mutex_exit(&is->sync);
  mx_mutex_exit(&mx_global_mutex);


#if MAL_OS_LINUX || MAL_OS_FREEBSD || MAL_OS_UDRV
  status = mx_ether_attach(is);
  if (status) {
    MX_WARN(("%s: failed to attach ethernet device, err=%d\n",
	     is->is_name, status));
  }
#endif

  myri_clock_init(is);

  mx_mutex_enter(&is->sync);
  return 0;

 abort_with_sync:
  mx_spin_lock_destroy(&is->snf.rx_intr_lock);
  mx_sync_destroy(&is->snf.kagent.sync);
  mx_sync_destroy(&is->snf.sync);
  mx_sync_destroy(&is->init_sync);
  mx_sync_destroy(&is->sync);
  mx_spin_lock_destroy(&is->cmd.spinlock);

  mx_kfree(is->mmu.host_table);

 abort_with_routes:
  mx_free_dma_page(is, &is->bogus.addr, &is->bogus.pin);

 abort_with_counters:
  mx_free_dma_page(is, &is->counters.addr, &is->counters.pin);

 abort_with_sysbox:
  mx_free_dma_page(is, &is->sysbox.addr, &is->sysbox.pin);

 abort_with_tx_done:
  mx_free_dma_page(is, &is->tx_done.addr, &is->tx_done.pin);

 abort_with_endpt_desc:
  mx_free_dma_page(is, &is->endpt_desc.addr, &is->endpt_desc.pin);

 abort_with_intr_ring:
  mx_free_intr(is);

 abort_with_rbc:
  mx_restore_pci_cap(is);

 abort_with_malloc:
  mx_kfree(is->es);

  mx_sync_destroy(&is->logging.sync);
  mx_sync_destroy(&is->dmabench.wait_sync);
  mx_spin_lock_destroy(&is->kreqq_spinlock);

 abort_with_host_query:
  is->board_ops.unmap(is);

 abort_with_nothing:

  return status;
}

int  
mx_instance_finalize (mx_instance_state_t *is)
{
  mx_mutex_enter(&is->sync);
  is->flags |= MX_IS_DEAD;
  mx_mutex_exit(&is->sync);

  mx_stop_mapper(is);

  mx_mutex_enter(&mx_global_mutex);
  mx_mutex_enter(&is->sync);

  if (mx_atomic_read(&is->ref_count)) {
    MX_WARN(("mx_instance_finalize: %ld refs remain returning EBUSY\n", 
	     (long)mx_atomic_read(&is->ref_count)));
    mx_mutex_exit(&is->sync);
    mx_mutex_exit(&mx_global_mutex);
    return EBUSY;
  }
#if MYRI_ENABLE_PTP
  if (is->ptp)
    myri_ptp_fini(is);
#endif
#if MAL_OS_LINUX || MAL_OS_FREEBSD || MAL_OS_UDRV
  mx_ether_detach(is);
#endif

  if (is->lanai.sram != NULL) {
    /*note -- this leaves the lanai in reset, so we can start to free
      things */
    mx_freeze_board(is);
  }

  mx_restore_pci_cap(is);


  mx_instances[is->id] = 0;
  mx_num_instances -= 1;
  mx_mutex_exit(&is->sync);
  mx_mutex_exit(&mx_global_mutex);
  mx_spin_lock_destroy(&is->cmd.spinlock);

  mx_sync_destroy(&is->sync);
  mx_sync_destroy(&is->init_sync);
  mx_sync_destroy(&is->snf.sync);

  myri_clock_destroy(is);

  if (is->es) {
    mx_kfree(is->es);
    mx_sync_destroy(&is->logging.sync);
    mx_sync_destroy(&is->dmabench.wait_sync);
    mx_spin_lock_destroy(&is->kreqq_spinlock);
  }
  is->es = 0;

  mx_free_dma_page(is, &is->bogus.addr, &is->bogus.pin);
  mx_free_dma_page(is, &is->counters.addr, &is->counters.pin);
  mx_free_dma_page(is, &is->sysbox.addr, &is->sysbox.pin);
  mx_free_dma_page(is, &is->tx_done.addr, &is->tx_done.pin);
  mx_free_dma_page(is, &is->endpt_desc.addr, &is->endpt_desc.pin);
  mx_free_intr(is);
  if (is->mmu.host_table != 0) {
    mx_kfree(is->mmu.host_table);
    is->mmu.host_table = 0;
  }

  is->mcp_print_len = 0;
  mx_kfree(is->mcp_print_buffer);
  is->mcp_print_buffer = NULL;
  if (is->lanai.sram != NULL) {
    /* unmap the board */
    is->board_ops.unmap(is);
  }
  return 0;
}

int
mx_mmap_off_to_kva(mx_endpt_state_t *es, unsigned long req, void **kva,
		   int *mem_type, mx_page_pin_t **pin)
{
  unsigned long off = 0;
  unsigned long abs_req = req;
  void *tmpkva;
  int index;
  int rc;

  rc = 0; /* force no-unused */

  /* mapping 1: (off <= sendq_size) gets the sendq */
  tmpkva = (void *)((char *)es->sendq.addr + (req - off));
  
  index = (req - off) / PAGE_SIZE;

  off += (unsigned long)es->sendq.size;
  if (req < off) {
    *kva = (void *)(uintptr_t)(es->sendq.pins[index].va);
    *pin = &es->sendq.pins[index];
    *mem_type = MX_MEM_HOSTMEM;
    return 0;
  }

  /* mapping 2: (off <= recvq_size) gets the recvq */
  
  index = (req - off) / PAGE_SIZE;
  off += (unsigned long)es->recvq.size;
  if (req < off) {
    *mem_type = MX_MEM_HOSTMEM;
    *kva = (void *)(uintptr_t)(es->recvq.pins[index].va);
    *pin = &es->recvq.pins[index];
    return 0;
  }

  /* mapping 3: (off <= eventq_size) gets the eventq */
  tmpkva = (void *)((char *)es->eventq.addr + (req - off));

  index = (req - off) / PAGE_SIZE;
  off += (unsigned long)es->eventq.size;
  if (req < off) {
    *kva = (void *)(uintptr_t)(es->eventq.pins[index].va);
    *pin = &es->eventq.pins[index];
    *mem_type = MX_MEM_HOSTMEM;
    return 0;
  }

  /* mapping 4: (off <= mmap_sram_size) gets the mmaped sram */
  tmpkva = (void *)((char *)es->user_mmapped_sram.addr + (req - off));

  off += (unsigned long)es->user_mmapped_sram.size;
  if (req < off) {
    *kva = tmpkva;
    *mem_type = MX_MEM_SRAM;
    return 0;
  }

  /* mapping 5: (off <= mmap_zereq_size) gets the ze req window */
  tmpkva = (void *)((char *)es->user_mmapped_zreq.addr + (req - off));

  off += (unsigned long)es->user_mmapped_zreq.size;
  if (req < off) {
    *kva = tmpkva;
    *mem_type = MX_MEM_SRAM;
    return 0;
  }

  /* mapping 6: (off <= kernel_vars_size) gets the kernel window (jiffies) */
  off += es->is->kernel_window ? MX_PAGE_SIZE : 0;
  if (req + MX_PAGE_SIZE == off && es->is->kernel_window) {
    *kva = es->is->kernel_window;
    *mem_type = MX_MEM_HOSTMEM;
    return 0;
  }

  /* mapping 7: kernel per-endpoint flow page */
  off += es->flow_window ? MX_PAGE_SIZE : 0;
  if (req + MX_PAGE_SIZE == off && es->flow_window) {
    *kva = es->flow_window;
    *pin = &es->flow_window_pin;
    *mem_type = MX_MEM_HOSTMEM;
    return 0;
  }

  /* mapping 8: PIO per-endpoint TX completion counters */
  off += es->is->tx_done.addr ? MX_PAGE_SIZE : 0;
  if (req + MX_PAGE_SIZE == off && es->is->tx_done.addr) {
    *kva = es->is->tx_done.addr;
    *pin = &es->is->tx_done.pin;
    *mem_type = MX_MEM_HOSTMEM;
    return 0;
  }

  if (es->es_type == MYRI_ES_SNF_RX) {
    rc = myri_snf_mmap_off_to_kva(es, req, &off, kva, mem_type, pin);
    if (rc == 0 || rc != ENOENT)
      return rc;
  }

 if (!es->privileged)
    return EPERM;
  
  /* mapping 6: [ 0x8000_0000, +16MB[ gets the MMIO space */
  off = (1U << 31);
  tmpkva = (void *)((char *)es->is->lanai.sram + (abs_req - off));

  if (abs_req >= off && abs_req < off + (unsigned long)0x1000000) {
    *kva = tmpkva;
    *mem_type = MX_MEM_SRAM;
    return 0;
  }

#ifdef notyet
  /* mapping 7: (off > (1U << 31)+128M) gets the specials */
  off = (1U << 31) + 128*1024*1024;
  tmpkva = (void *)((char *)es->is->lanai.special_regs + (req - off));  
  off += (unsigned long)es->is->specials_size;
  if (req < off) {
    *kva = tmpkva;
    *mem_type = MX_MEM_SPECIAL;
    return 0;
  }

#endif

  return EINVAL;

}

/* returns:
   - 1 if an error has been handled, either by marking
   the board dead or by recovering the error
   - 0 if not parity error was present

   The return value does not reflect whether the error was recovered or not.
   mx_is_dead(is) would reflect that anyway.
 */
static int
mx_handle_parity_error(mx_instance_state_t *is)
{
  volatile uint64_t *mem;
  volatile uint64_t *end;
  volatile uint64_t dont_care;
  int status, ms, do_ether_reattach, i;
  int found_inside_critical = 0;
  int board_ok = 0;
  uint32_t parity_status, endpoint_bitmap;
  mx_sync_t tmp_sync;
  int nb_active_ep;

  /* by default, only recover parity errors */
  if (is->board_ops.detect_parity_error(is) == 0 
      && !myri_recover_from_all_errors)
    return 0;

  /* Get ready for application telling us that they are ready */
  mx_sync_init(&is->ready_for_recovery_sync, is, 0, "ready_for_recovery sync");
  /* Temporary sync for commands */
  mx_sync_init(&tmp_sync, is, 0, "parity recovery temp sync");  
  
  /* mark the board dead and eventually in recovery */
  is->flags |= MX_IS_DEAD;
  if (myri_parity_recovery)
    is->flags |= MX_PARITY_RECOVERY;
  is->saved_state.reason = MX_DEAD_RECOVERABLE_SRAM_PARITY_ERROR;

  /* Wake the raw interface so it knows about the parity */
  mx_mutex_enter(&is->sync);
  if (is->raw.es)
    mx_wake(&is->raw.sync);
  mx_mutex_exit(&is->sync);

  MX_WARN(("%s: Parity error detected on board %d\n", is->is_name, is->id));

  /* the code below is very specific to LX, don't do it on ZE */
  if (!myri_parity_recovery)
    goto out_with_syncs;
  myri_parity_recovery -= 1;

  /* drop the global mutex */
  mx_mutex_exit(&mx_global_mutex);  

  /* sleep for a little while to give the mcp a chance to notice it
     and start spinning on REQ_ACK_1 before we clear the parity error bit */
  mx_spin(20000);

  /* Check to see if mcp really was doing parity recovery by
     checking to see if REQ_ACK_1 is set */
  if (!MAL_IS_ZE_BOARD(is->board_type)
      && (mx_read_lanai_special_reg_u32(is, lx.ISR) & MX_LX_REQ_ACK_1) == 0
      && !myri_recover_from_all_errors) {
    /* Nope, so we're screwed.  The parity error could have
       been anything */
    MX_WARN(("\t Unrecoverable parity error without scrubber\n"));
    goto out_without_global_mutex;
  }

  /* Terminate pending lanai command */
  if (MAL_IS_ZE_BOARD(is->board_type)) {
    is->board_ops.park(is);
    parity_status = MCP_PARITY_RECOVER_ALL;
  } else {
    /* clear it */
    mx_write_lanai_isr(is, MX_LX_PARITY_INT);

    /* scan critical region and see what we've got */
    mem = (volatile uint64_t *)is->parity_critical_start;
    end = (volatile uint64_t *)is->parity_critical_end;
    while(mem != end) {
      dont_care = *mem;
      MAL_STBAR();
#if MAL_OS_UDRV
      if (mx_lxgdb && ((((uintptr_t) mem) - ((uintptr_t) is->lanai.sram)) & ~7) 
	  == mx_read_lanai_special_reg_u32(is, lx.AISR))
#else
	if ((mx_read_lanai_special_reg_u32(is, lx.ISR) & MX_LX_PARITY_INT) != 0)
#endif
	  {
	    /* clear it */
	    mx_write_lanai_isr(is, MX_LX_PARITY_INT);
	    MX_WARN(("\t Parity error found at LANai SRAM address 0x%x\n",
		     (int)((uintptr_t)mem - (uintptr_t)is->lanai.sram)));
	    found_inside_critical++;
	  }
      mem++;
    }
    
    MX_WARN(("\t Scan completed:  found_inside_critical = %d\n",
	     found_inside_critical));

    if (found_inside_critical) {
      /* We're screwed.  Since there is an error in the mcp's scrubber,
	 we can't recover.  */
      MX_WARN(("\t Unrecoverable parity error(s)\n"));
      goto out_without_global_mutex;
    }

    MX_WARN(("\t Handshaking with firmware for parity error recovery\n"));
    /* handshake with the mcp and let him figure it out */
    mx_write_lanai_isr(is, MX_LX_REQ_ACK_1);
    MAL_STBAR();

    ms = 0;
    do {
      mx_sleep(&tmp_sync, MX_SMALL_WAIT, MX_SLEEP_NOINTR);
      ms += MX_SMALL_WAIT;
      parity_status = MCP_GETVAL(is, parity_status);
    } while (parity_status == MCP_PARITY_NONE && ms <= MCP_INIT_TIMEOUT);

    /* when recovering from all errors, simulate a normal parity */
    if (parity_status == MCP_PARITY_NONE && myri_recover_from_all_errors) {
      parity_status = MCP_PARITY_RECOVER_ALL;
      ms = 0;
    }
  
    if (parity_status == MCP_PARITY_MULTIPLE) {
      MX_WARN(("\t Multiple parity errors detected\n"));
      goto out_without_global_mutex;
    }
  
    if (parity_status == MCP_PARITY_PANIC) {
      MX_WARN(("\t Unrecoverable parity error detected\n"));
      goto out_without_global_mutex;
    }
  
    if (ms > MCP_INIT_TIMEOUT) {
      MX_WARN(("\t Timeout for parity error recovery, assuming not recoverable\n"));
      goto out_without_global_mutex;
    }
  }
  
  MX_WARN(("\t Recoverable Parity error:  Stopping mapper and ethernet\n"));

  do_ether_reattach = mx_ether_parity_detach(is);
  
  /* Kill any running mapper */
  mx_stop_mapper(is);

  mx_mutex_enter(&is->sync);
  is->parity_errors_detected++;
  mx_mutex_exit(&is->sync);

  nb_active_ep = 0;
  for (i = 0; i < myri_max_endpoints; i++) {
    mx_endpt_state_t * es = mx_get_endpoint(is, i);
    if (es != NULL) {
      nb_active_ep += 1;
      es->flags |= MX_ES_RECOVERY;
      mx_put_endpoint(es);
    }
  }
  
  /* wait until everybody waits for recovery */
  MX_INFO(("Waiting for all endpoints to be ready for recovery...\n"));
  for (i = 0; i < nb_active_ep; i++) {
    mx_sleep(&is->ready_for_recovery_sync, MX_MAX_WAIT, 0);
  }
  
  for (endpoint_bitmap = 0, i = 0; i < myri_max_endpoints; i++) {
    if (is->es[i] != NULL)
      endpoint_bitmap |= (1 << i);
  }
  
  if (parity_status == MCP_PARITY_RECOVER_ALL) {
    is->parity_errors_corrected++;
  } else {
    mal_assert(parity_status == MCP_PARITY_RECOVER_NONE);
    MX_INFO(("Possible parity contamination, all endpoints terminated\n"));
    endpoint_bitmap = 0;
  }

  mx_enable_pci_config_command_bit(is, MX_PCI_COMMAND_MEMORY); 
  MX_WARN(("\t Reloading firmware, endpoint_bitmap = 0x%x \n", endpoint_bitmap));
  
#ifdef MX_HAS_INTR_RECOVER
  status = mx_intr_recover(is);
  if (status) {
    MX_WARN(("Could not recover interrupts\n"));
    board_ok = 0;
    goto out_without_global_mutex;
  }
#endif
  status = mx_init_board(is, 1, endpoint_bitmap);
  if (status) {
    MX_WARN(("Could not re-init the board\n"));
    board_ok = 0;
    goto out_without_global_mutex;
  }

  mx_mutex_enter(&is->sync);
  is->flags &= ~MX_PARITY_RECOVERY;
  is->flags &= ~MX_IS_DEAD;
  is->saved_state.reason = 0;
  mx_mutex_exit(&is->sync);

  MX_WARN(("\t Firwmare has been reloaded.  Re-starting kernel services \n"));


  /* re-attach ethernet interface */
  if (do_ether_reattach)
    mx_ether_parity_reattach(is);

  MX_WARN(("\t Finished recovering from parity error \n"));
  board_ok = 1;

  /* wake up all the endpoints */
  MX_INFO(("Restoring all applications...\n"));
  for (i = 0; i < myri_max_endpoints; i++) {
    mx_endpt_state_t * es = mx_get_endpoint(is, i);
    if (es != NULL) {
      if (es->flags & MX_ES_RECOVERY) {
	es->flags &= ~MX_ES_RECOVERY;
	mx_wake(&es->recovery_sync);
      }
      mx_put_endpoint(es);
    }
  }

 out_without_global_mutex:
  /* return to the caller with the global mutex held */
  mx_mutex_enter(&mx_global_mutex);  

 out_with_syncs:
  mx_sync_destroy(&tmp_sync);
  mx_sync_destroy(&is->ready_for_recovery_sync);

  if (!board_ok) {
    MX_WARN(("Please reboot to clear the problem!!\n"));
    mx_mark_board_dead(is, MX_DEAD_SRAM_PARITY_ERROR, 0);
  }
  return 1;
}

void
mx_watchdog_body(void)
{
  int i;
  uint32_t uptime;
  mx_instance_state_t *is;
  static int limit = 10;

  mx_mutex_enter(&mx_global_mutex);
  for (i = 0; i < myri_max_instance; ++i) {
    uint16_t pci_status, pci_cmd_reg;
    is = mx_instances[i];
    if (!is || mx_is_dead(is))
	continue;
    if (mx_read_pci_config_16(is, offsetof(mx_pci_config_t, Status), &pci_status) != 0
	|| pci_status == 0xffff) {
      MX_WARN(("Board number %d conf space unavailable\n", is->id));
      if (!MAL_IS_ZE_BOARD(is->board_type) || mx_handle_parity_error(is) == 0)
	mx_mark_board_dead(is, MX_DEAD_NIC_RESET, 0);
      continue;
    }
    if (pci_status & MX_PCI_STATUS_PERR) {
      MX_WARN(("Board number %d has PCI parity error\n", is->id));
      mx_mark_board_dead(is, MX_DEAD_PCI_PARITY_ERROR, 0);
      continue;
    }
    if (pci_status & MX_PCI_STATUS_MABORT) {
      MX_WARN(("Board number %d has PCI Master Abort\n", is->id));
      mx_mark_board_dead(is, MX_DEAD_PCI_MASTER_ABORT, 0);
      continue;
    }
#ifdef MX_HAS_BRIDGE_PCI_SEC_STATUS
    if (mx_bridge_pci_sec_status(is) & MX_PCI_STATUS_PERR) {
      MX_WARN(("Board number %d: our PCI bridge is signalling a PCI parity error\n", is->id));
      mx_mark_board_dead(is, MX_DEAD_PCI_PARITY_ERROR, 1);
      continue;
    }
#endif
    /* On 10G, parity-recovery through mx_handle_parity_error() is
       only attempted if we lost config-space, 
       Loosing config-space on 2G is a fatal-error
    */
    if (mx_read_pci_config_16(is, offsetof(mx_pci_config_t, Command), &pci_cmd_reg) != 0
	|| pci_cmd_reg == 0xffff || !(pci_cmd_reg & MX_PCI_COMMAND_MASTER)) {
      MX_WARN(("Board number %d lost Master-Enable bit\n", is->id));
      if (!MAL_IS_ZE_BOARD(is->board_type) || mx_handle_parity_error(is) == 0)
	mx_mark_board_dead(is, MX_DEAD_NIC_RESET, 0);
      continue;
    }

    uptime = ntohl(*(uint32_t *)is->counters.addr);
    if (uptime == 0) {
      limit--;
      if (limit > 0)
	continue;
    }
    /* if the MCP uptime has stopped on the host, check it on the MCP itself */
    if (uptime == is->old_lanai_uptime)
      uptime = ntohl(*is->counters.mcp);
    
    if (uptime == is->old_lanai_uptime) {
      MX_WARN(("Board number %d stopped with %d seconds uptime\n",
	       is->id, uptime));
      /* trying parity-recovery based on uptime is only for 2G 
	 (loosing config-space is the criteria for 10G */
      if (MAL_IS_ZE_BOARD(is->board_type) || mx_handle_parity_error(is) == 0) {
#if MAL_OS_UDRV
	if (mx_lxgdb)
	  MX_WARN(("Board number %d seems to be stopped, moving on though\n",
		   is->id));
	else
#endif
	  mx_mark_board_dead(is, MX_DEAD_WATCHDOG_TIMEOUT, uptime);
      }
    }
    is->old_lanai_uptime = uptime;
    /* Ensure that we regularly poll statistics so that we can reliably do the
     * 32 to 64 bit counter conversion */
    {
      myri_snf_stats_t stats;
      myri_snf_get_stats(is, &stats);
    }
#if MAL_OS_LINUX
    if (is->ether)
      mx_ether_watchdog(is);
#endif
  }
  mx_mutex_exit(&mx_global_mutex);
}

/*
 * The mcp died.  Preserve as much information as possible,
 * and put the nic into a safe state.
 */

void
mx_mark_board_dead(mx_instance_state_t *is, int reason, int arg)
{
  /* make sure that we're the first one to discover
     a problem, and flag that the board is dead to
     prevent its use */
  mx_mutex_enter(&is->sync);
  if (reason != MX_DEAD_SRAM_PARITY_ERROR && mx_is_dead(is)) {
    mx_mutex_exit(&is->sync);
    return;
  }
  is->flags |= MX_IS_DEAD;
  mx_mutex_exit(&is->sync);

  if (reason == MX_DEAD_COMMAND_TIMEOUT ||
      reason == MX_DEAD_ENDPOINT_CLOSE_TIMEOUT) {
    mx_dump_interrupt_ring(is);
  }

  /* usually timeouts are designed that a parity error won't be
     mistaken for a timeout before arriving here, but just in case,
     let's make sure */
  if (reason != MX_DEAD_SRAM_PARITY_ERROR && 
      is->board_ops.detect_parity_error(is)) {
    MX_WARN(("%s: NIC has Internal Parity Error (driver reason is %d)\n", 
	     is->is_name, reason));
  }

  mx_parse_mcp_error(is);

  is->saved_state.reason = reason;
  is->saved_state.arg = arg;

  is->board_ops.park(is);

  /* Wake the raw interface so it knows about the board dying */
  mx_mutex_enter(&is->sync);
  if (is->raw.es)
    mx_wake(&is->raw.sync);
  mx_mutex_exit(&is->sync);

  MX_WARN(("Board number %d marked dead\n", is->id));
}

#ifndef MX_HAS_MAP_PCI_SPACE
void *
mx_map_pci_space (mx_instance_state_t * is, int bar, uint32_t offset, uint32_t len)

{
  if (bar)
    return NULL;
  else
    return mx_map_io_space(is, offset, len);
}
#endif
