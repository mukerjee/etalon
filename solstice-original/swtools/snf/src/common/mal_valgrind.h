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

#ifndef _mal_valgrind_h_
#define _mal_valgrind_h_

#include "mal.h"
#include "mal_io.h"
#include "myri_raw.h"

#if MAL_DEBUG && MAL_VALGRIND && !defined MAL_KERNEL

/*
 * Valgrind support to check memory access and allocation.
 * Use "valgrind --weird-hacks=lax-ioctls myprogram" to check your program.
 *  
 * The option name has changed in more recent valgrind versions, use:
 *      "valgrind --sim-hints=lax-ioctls myprogram"
 */

#include "valgrind/memcheck.h"

/* Mark a memory buffer as non-accessible, accessible or accessible+initialized */
#define MAL_VALGRIND_MEMORY_MAKE_NOACCESS(p, s) VALGRIND_MAKE_MEM_NOACCESS(p, s)
#define MAL_VALGRIND_MEMORY_MAKE_WRITABLE(p, s) VALGRIND_MAKE_MEM_UNDEFINED(p, s)
#define MAL_VALGRIND_MEMORY_MAKE_READABLE(p, s) VALGRIND_MAKE_MEM_DEFINED(p, s)

/* Check that input buffers are readable and output buffers are writable */
static inline void
MAL_VALGRIND_PRE_IOCTL_CHECK(int cmd, void *buffer)
{
  switch (cmd) {

  /* IOCTL without any argument */
  case MX_SET_RAW:
  case MX_SET_ROUTE_BEGIN:
  case MX_SET_ROUTE_END:
  case MX_WAKE:
  case MX_APP_WAKE:
    break;

  /* IOCTL with an input uint32_t */
  case MX_DEREGISTER:
  case MX_ARM_TIMER:
    VALGRIND_CHECK_MEM_IS_DEFINED(buffer, sizeof(uint32_t));
    break;

  /* IOCTL with an output uint32_t */
  case MYRI_GET_BOARD_MAX:
  case MYRI_GET_BOARD_COUNT:
  case MX_GET_MAX_SEND_HANDLES:
  case MYRI_GET_ENDPT_MAX:
  case MX_GET_MAX_PEERS:
  case MX_GET_SMALL_MESSAGE_THRESHOLD:
  case MX_GET_MEDIUM_MESSAGE_THRESHOLD:
  case MX_GET_MAX_RDMA_WINDOWS:
  case MX_GET_BOARD_STATUS:
    VALGRIND_CHECK_MEM_IS_ADDRESSABLE(buffer, sizeof(uint32_t));
    break;

  /* IOCTL with an input/output uint32_t */
  case MYRI_GET_BOARD_TYPE:
  case MYRI_GET_BOARD_NUMA_NODE:
  case MYRI_GET_PORT_COUNT:
  case MX_GET_SERIAL_NUMBER:
    VALGRIND_CHECK_MEM_IS_DEFINED(buffer, sizeof(uint32_t));
    VALGRIND_CHECK_MEM_IS_ADDRESSABLE(buffer, sizeof(uint32_t));
    break;

  case MX_SET_ENDPOINT:
    {
      mx_set_endpt_t *x = (mx_set_endpt_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->endpoint);
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->session_id, sizeof(x->session_id));
      break;
    }

  case MX_WAIT:
  case MX_APP_WAIT:
    {
      mx_wait_t *x = (mx_wait_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->timeout);
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->mcp_wake_events, sizeof(x->mcp_wake_events));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->status, sizeof(x->status));
      break;
    }

  case MX_GET_COPYBLOCKS:
    {
      mx_get_copyblock_t *x = (mx_get_copyblock_t *) buffer;
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->sendq_offset, sizeof(x->sendq_offset));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->sendq_len, sizeof(x->sendq_len));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->recvq_offset, sizeof(x->recvq_offset));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->recvq_len, sizeof(x->recvq_len));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->eventq_offset, sizeof(x->eventq_offset));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->eventq_len, sizeof(x->eventq_len));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->user_mmapped_sram_offset, sizeof(x->user_mmapped_sram_offset));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->user_mmapped_sram_len, sizeof(x->user_mmapped_sram_len));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->user_mmapped_zreq_offset, sizeof(x->user_mmapped_zreq_offset));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->user_mmapped_zreq_len, sizeof(x->user_mmapped_zreq_len));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->user_reqq_offset, sizeof(x->user_reqq_offset));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->user_reqq_len, sizeof(x->user_reqq_len));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->user_dataq_offset, sizeof(x->user_dataq_offset));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->user_dataq_len, sizeof(x->user_dataq_len));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->kernel_window_offset, sizeof(x->kernel_window_offset));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->kernel_window_len, sizeof(x->kernel_window_len));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->flow_window_offset, sizeof(x->flow_window_offset));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->flow_window_len, sizeof(x->flow_window_len));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->send_compcnt_offset, sizeof(x->send_compcnt_offset));
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->send_compcnt_len, sizeof(x->send_compcnt_len));
      break;
    }

  case MYRI_GET_NIC_ID:
    {
      myri_get_nic_id_t *x = (myri_get_nic_id_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->board);
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->nic_id, sizeof(x->nic_id));
      break;
    }

  case MX_REGISTER:
    {
      mx_reg_t *x = (mx_reg_t *) buffer;
      uint32_t nsegs = x->nsegs;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->rdma_id);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->memory_context);
      VALGRIND_CHECK_VALUE_IS_DEFINED(nsegs);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->segs.vaddr);
      if (nsegs == 1) {
	VALGRIND_CHECK_VALUE_IS_DEFINED(x->segs.len);
      } else {
	mx_reg_seg_t * segs = (mx_reg_seg_t *)(uintptr_t) x->segs.vaddr;
	int i;
	for(i=0; i<nsegs; i++) {
	  VALGRIND_CHECK_VALUE_IS_DEFINED(segs[i].vaddr);
	  VALGRIND_CHECK_VALUE_IS_DEFINED(segs[i].len);
	}
      }
      break;
    }

  case MX_NIC_ID_TO_PEER_INDEX:
    {
      mx_lookup_peer_t *x = (mx_lookup_peer_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->nic_id);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->board_number);
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->index, sizeof(x->index));
      break;
    }

  case MX_PEER_INDEX_TO_NIC_ID:
    {
      mx_lookup_peer_t *x = (mx_lookup_peer_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->index);
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->nic_id, sizeof(x->nic_id));
      break;
    }

  case MX_NIC_ID_TO_HOSTNAME:
    {
      mx_nic_id_hostname_t *x = (mx_nic_id_hostname_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->nic_id);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->va);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->len);
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(x->va, x->len);
      break;
    }

  case MX_HOSTNAME_TO_NIC_ID:
    {
      mx_nic_id_hostname_t *x = (mx_nic_id_hostname_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->va);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->len);
      VALGRIND_CHECK_MEM_IS_DEFINED(x->va, x->len);
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->nic_id, sizeof(x->nic_id));
      break;
    }

  case MX_WAKE_ENDPOINT:
    {
      mx_wake_endpt_t *x = (mx_wake_endpt_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->endpt);
      break;
    }

  case MYRI_GET_VERSION:
    {
      myri_get_version_t *x = (myri_get_version_t *) buffer;
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(x, sizeof(*x));
      break;
    }

  case MX_DIRECT_GETV:
    {
      mx_direct_getv_t *x = (mx_direct_getv_t *) buffer;
      uint32_t src_nsegs = x->src_nsegs;
      uint32_t dst_nsegs = x->dst_nsegs;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->length);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->src_board_num);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->src_endpt);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->src_session);
      VALGRIND_CHECK_VALUE_IS_DEFINED(src_nsegs);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->src_segs.vaddr);
      if (src_nsegs == 1) {
	VALGRIND_CHECK_VALUE_IS_DEFINED(x->src_segs.len);
      } else {
	/* no way to check src since it's in another address space */
      }
      VALGRIND_CHECK_VALUE_IS_DEFINED(dst_nsegs);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->dst_segs.vaddr);
      if (dst_nsegs == 1) {
	VALGRIND_CHECK_VALUE_IS_DEFINED(x->dst_segs.len);
      } else {
	mx_shm_seg_t * dst_segs = (mx_shm_seg_t *)(uintptr_t) x->dst_segs.vaddr;
	int i;
	for(i=0; i<dst_nsegs; i++) {
	  VALGRIND_CHECK_VALUE_IS_DEFINED(dst_segs[i].vaddr);
	  VALGRIND_CHECK_VALUE_IS_DEFINED(dst_segs[i].len);
	}
      }
      break;
    }


  case MYRI_RAW_SEND:
    {
      myri_raw_send_t *x = (myri_raw_send_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->data_pointer);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->route_pointer);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->context);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->route_length);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->physical_port);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->data_length);
      break;
    }

  case MYRI_RAW_GET_NEXT_EVENT:
    {
      myri_raw_next_event_t *x = (myri_raw_next_event_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->recv_buffer);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->recv_bytes);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->timeout);
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->status, sizeof(x->status));
      /* TODO */
      break;
    }


  case MX_SET_MAPPER_STATE:
    {
      mx_mapper_state_t *x = (mx_mapper_state_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->board_number);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->mapper_mac);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->iport);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->map_version);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->num_hosts);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->network_configured);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->routes_valid);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->level);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->flags);
      break;
    }

  case MX_GET_PRODUCT_CODE:
    {
      mx_get_eeprom_string_t *x = (mx_get_eeprom_string_t *) buffer;
      char * ptr = (char *)((uintptr_t) (x->buffer));
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->board_number);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->buffer);
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(ptr, MYRI_MAX_STR_LEN);
      break;
    }

  case MX_NIC_ID_TO_BOARD_NUM:
    {
      mx_nic_id_to_board_num_t *x = (mx_nic_id_to_board_num_t *)buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->nic_id);
      VALGRIND_CHECK_MEM_IS_ADDRESSABLE(&x->board_number, sizeof(x->board_number));
      break;
    }

  case MX_SET_PEER_NAME:
    {
      mx_nic_id_hostname_t *x = (mx_nic_id_hostname_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->nic_id);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->va);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->len);
      VALGRIND_CHECK_MEM_IS_DEFINED(x->va, x->len);
      break;
    }

  case MYRI_PCI_CFG_READ:
    {
      myri_pci_cfg_t *x = (myri_pci_cfg_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->board);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->offset);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->width);
    }
    break;
  case MYRI_PCI_CFG_WRITE:
    {
      myri_pci_cfg_t *x = (myri_pci_cfg_t *) buffer;
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->board);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->offset);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->width);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->val);
    }
    break;

  case MYRI_SOFT_RX:
    {
      myri_soft_rx_t *x = (myri_soft_rx_t*)buffer;
      int i;

      VALGRIND_CHECK_VALUE_IS_DEFINED(x->pkt_len);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->hdr_len);
      //VALGRIND_CHECK_VALUE_IS_DEFINED(x->csum);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->flags);
      VALGRIND_CHECK_VALUE_IS_DEFINED(x->seg_cnt);
      if (x->seg_cnt == 1) {
	VALGRIND_CHECK_VALUE_IS_DEFINED(x->copy_desc[0].ptr);
	VALGRIND_CHECK_VALUE_IS_DEFINED(x->copy_desc[0].len);
      }
      break;
    }


  /* TODO */
  case MYRI_GET_INTR_COAL:
  case MYRI_SET_INTR_COAL:
  case MYRI_GET_COUNTERS_CNT:
  case MYRI_GET_COUNTERS_STR:
  case MYRI_GET_COUNTERS_VAL:
  case MYRI_GET_COUNTERS_IRQ:
  case MYRI_GET_COUNTERS_KB:
  case MYRI_CLEAR_COUNTERS:
  case MYRI_GET_LOGGING:
  case MYRI_GET_SRAM_SIZE:

#if MAL_OS_UDRV
  case	MYRI_UDRV_DOORBELL:
#endif


  case MX_PIN_SEND:
  case MX_GET_MAPPER_MSGBUF_SIZE:
  case MX_GET_MAPPER_MAPBUF_SIZE:
  case MX_GET_MAPPER_MSGBUF:
  case MX_GET_MAPPER_MAPBUF:
  case MX_GET_PEER_FORMAT:
  case MX_GET_ROUTE_SIZE:
  case MX_GET_PEER_TABLE:
  case MX_GET_ROUTE_TABLE:
  case MX_PAUSE_MAPPER:
  case MX_RESUME_MAPPER:
  case MYRI_GET_ENDPT_OPENER:
  case MYRI_MMAP:
  case MX_CLEAR_PEER_NAMES:
  case MX_SET_HOSTNAME:
  case MX_GET_CACHELINE_SIZE:
  case MX_GET_LINK_STATE:
  case MX_PIN_RECV:
  case MX_RUN_DMABENCH:
  case MX_CLEAR_WAIT:
  case MYRI_RAW_GET_PARAMS:
  case MX_CLEAR_ROUTES:
  case MX_GET_CPU_FREQ:
  case MX_GET_PCI_FREQ:
  case MX_GET_MAPPER_STATE:
  case MX_RECOVER_ENDPOINT:
  case MX_REMOVE_PEER:
  case MX_DIRECT_GET:
  case MX_GET_PART_NUMBER:
  case MX_IOCTL_ND_COMPLETE_OVERLAPPED:
  case MX_WAIT_FOR_RECOVERY:
  case MYRI_SNF_SET_ENDPOINT_TX:
  case MYRI_SNF_SET_ENDPOINT_RX:
  case MYRI_SNF_SET_ENDPOINT_RX_RING:
  case MYRI_SNF_RX_START:
  case MYRI_SNF_RX_STOP:
  case MYRI_SNF_RX_RING_PARAMS:
  case MYRI_SNF_RX_NOTIFY:
  case MYRI_SNF_STATS:
  case MYRI_SNF_WAIT:
    VALGRIND_PRINTF("Unhandled PRE IOCTL %x\n", cmd);
    break;
  default:
    VALGRIND_PRINTF("Unknown PRE IOCTL %x\n", cmd);
  }
}

/* Mark that output buffers as readable */
static inline void
MAL_VALGRIND_POST_IOCTL_CHECK(int cmd, void *buffer)
{
  switch (cmd) {

  /* IOCTL without any argument */
  case MX_SET_RAW:
  case MX_SET_ROUTE_BEGIN:
  case MX_SET_ROUTE_END:
  case MX_WAKE:
  case MX_APP_WAKE:
    break;

  /* IOCTL with an input uint32_t */
  case MX_DEREGISTER:
  case MX_ARM_TIMER:
    break;

  /* IOCTL with an output uint32_t */
  case MYRI_GET_BOARD_MAX:
  case MYRI_GET_BOARD_COUNT:
  case MX_GET_MAX_SEND_HANDLES:
  case MYRI_GET_ENDPT_MAX:
  case MX_GET_MAX_PEERS:
  case MX_GET_SMALL_MESSAGE_THRESHOLD:
  case MX_GET_MEDIUM_MESSAGE_THRESHOLD:
  case MX_GET_MAX_RDMA_WINDOWS:
  case MX_GET_BOARD_STATUS:
    VALGRIND_MAKE_MEM_DEFINED(buffer, sizeof(uint32_t));
    break;

  /* IOCTL with an input/output uint32_t */
  case MYRI_GET_BOARD_TYPE:
  case MYRI_GET_BOARD_NUMA_NODE:
  case MYRI_GET_PORT_COUNT:
  case MX_GET_SERIAL_NUMBER:
    VALGRIND_MAKE_MEM_DEFINED(buffer, sizeof(uint32_t));
    break;

  case MX_SET_ENDPOINT:
    {
      mx_set_endpt_t *x = (mx_set_endpt_t *) buffer;
      VALGRIND_MAKE_MEM_DEFINED(&x->session_id, sizeof(x->session_id));
      break;
    }

  case MX_WAIT:
  case MX_APP_WAIT:
    {
      mx_wait_t *x = (mx_wait_t *) buffer;
      VALGRIND_MAKE_MEM_DEFINED(&x->mcp_wake_events, sizeof(x->mcp_wake_events));
      VALGRIND_MAKE_MEM_DEFINED(&x->status, sizeof(x->status));
      break;
    }

  case MX_GET_COPYBLOCKS:
    {
      mx_get_copyblock_t *x = (mx_get_copyblock_t *) buffer;
      VALGRIND_MAKE_MEM_DEFINED(&x->sendq_offset, sizeof(x->sendq_offset));
      VALGRIND_MAKE_MEM_DEFINED(&x->sendq_len, sizeof(x->sendq_len));
      VALGRIND_MAKE_MEM_DEFINED(&x->recvq_offset, sizeof(x->recvq_offset));
      VALGRIND_MAKE_MEM_DEFINED(&x->recvq_len, sizeof(x->recvq_len));
      VALGRIND_MAKE_MEM_DEFINED(&x->eventq_offset, sizeof(x->eventq_offset));
      VALGRIND_MAKE_MEM_DEFINED(&x->eventq_len, sizeof(x->eventq_len));
      VALGRIND_MAKE_MEM_DEFINED(&x->user_mmapped_sram_offset, sizeof(x->user_mmapped_sram_offset));
      VALGRIND_MAKE_MEM_DEFINED(&x->user_mmapped_sram_len, sizeof(x->user_mmapped_sram_len));
      VALGRIND_MAKE_MEM_DEFINED(&x->user_mmapped_zreq_offset, sizeof(x->user_mmapped_zreq_offset));
      VALGRIND_MAKE_MEM_DEFINED(&x->user_mmapped_zreq_len, sizeof(x->user_mmapped_zreq_len));
      VALGRIND_MAKE_MEM_DEFINED(&x->user_reqq_offset, sizeof(x->user_reqq_offset));
      VALGRIND_MAKE_MEM_DEFINED(&x->user_reqq_len, sizeof(x->user_reqq_len));
      VALGRIND_MAKE_MEM_DEFINED(&x->user_dataq_offset, sizeof(x->user_dataq_offset));
      VALGRIND_MAKE_MEM_DEFINED(&x->user_dataq_len, sizeof(x->user_dataq_len));
      VALGRIND_MAKE_MEM_DEFINED(&x->kernel_window_offset, sizeof(x->kernel_window_offset));
      VALGRIND_MAKE_MEM_DEFINED(&x->kernel_window_len, sizeof(x->kernel_window_len));
      VALGRIND_MAKE_MEM_DEFINED(&x->flow_window_offset, sizeof(x->flow_window_offset));
      VALGRIND_MAKE_MEM_DEFINED(&x->flow_window_len, sizeof(x->flow_window_len));
      VALGRIND_MAKE_MEM_DEFINED(&x->send_compcnt_offset, sizeof(x->send_compcnt_offset));
      VALGRIND_MAKE_MEM_DEFINED(&x->send_compcnt_len, sizeof(x->send_compcnt_len));
      break;
    }

  case MYRI_GET_NIC_ID:
    {
      myri_get_nic_id_t *x = (myri_get_nic_id_t *) buffer;
      VALGRIND_MAKE_MEM_DEFINED(&x->nic_id, sizeof(x->nic_id));
      break;
    }

  case MX_REGISTER:
    break;

  case MX_NIC_ID_TO_PEER_INDEX:
    {
      mx_lookup_peer_t *x = (mx_lookup_peer_t *) buffer;
      VALGRIND_MAKE_MEM_DEFINED(&x->index, sizeof(x->index));
      break;
    }

  case MX_PEER_INDEX_TO_NIC_ID:
    {
      mx_lookup_peer_t *x = (mx_lookup_peer_t *) buffer;
      VALGRIND_MAKE_MEM_DEFINED(&x->nic_id, sizeof(x->nic_id));
      break;
    }

  case MX_NIC_ID_TO_HOSTNAME:
    {
      mx_nic_id_hostname_t *x = (mx_nic_id_hostname_t *) buffer;
      VALGRIND_MAKE_MEM_DEFINED(x->va, x->len);
      break;
    }

  case MX_HOSTNAME_TO_NIC_ID:
    {
      mx_nic_id_hostname_t *x = (mx_nic_id_hostname_t *) buffer;
      VALGRIND_MAKE_MEM_DEFINED(&x->nic_id, sizeof(x->nic_id));
      break;
    }

  case MX_WAKE_ENDPOINT:
    break;

  case MYRI_GET_VERSION:
    {
      myri_get_version_t *x = (myri_get_version_t *) buffer;
      VALGRIND_MAKE_MEM_DEFINED(x, sizeof(*x));
      break;
    }

  case MX_DIRECT_GETV:
    break;

  case MX_SET_ROUTE:
    break;

  case MYRI_RAW_SEND:
    break;

  case MYRI_RAW_GET_NEXT_EVENT:
    {
      myri_raw_next_event_t *x = (myri_raw_next_event_t *) buffer;
      VALGRIND_MAKE_MEM_DEFINED(&x->status, sizeof(x->status));
      if (x->status == MYRI_RAW_RECV_COMPLETE) {
	VALGRIND_MAKE_MEM_DEFINED(&x->incoming_port, sizeof(x->incoming_port));
	VALGRIND_MAKE_MEM_DEFINED(&x->recv_bytes, sizeof(x->recv_bytes));
	VALGRIND_MAKE_MEM_DEFINED(&x->recv_buffer, sizeof(x->recv_buffer));
	VALGRIND_MAKE_MEM_DEFINED(x->recv_buffer, x->recv_bytes);
      } else if (x->status == MYRI_RAW_SEND_COMPLETE) {
	VALGRIND_MAKE_MEM_DEFINED(&x->context, sizeof(x->context));
      }
      break;
    }
    
  case MX_SET_MAPPER_STATE:
  case MX_SET_NIC_REPLY_INFO:
    break;

  case MX_GET_PRODUCT_CODE:
    {
      mx_get_eeprom_string_t *x = (mx_get_eeprom_string_t *) buffer;
      char * ptr = (char *)((uintptr_t) (x->buffer));
      VALGRIND_MAKE_MEM_DEFINED(ptr, MYRI_MAX_STR_LEN);
      break;
    }

  case MX_NIC_ID_TO_BOARD_NUM:
    {
      mx_nic_id_to_board_num_t *x = (mx_nic_id_to_board_num_t *)buffer;
      VALGRIND_MAKE_MEM_DEFINED(&x->board_number, sizeof(x->board_number));
      break;
    }

  case MYRI_SOFT_RX: break;


    /* TODO */
  case MYRI_GET_INTR_COAL:
  case MYRI_SET_INTR_COAL:
  case MYRI_GET_COUNTERS_CNT:
  case MYRI_GET_COUNTERS_STR:
  case MYRI_GET_COUNTERS_VAL:
  case MYRI_GET_COUNTERS_IRQ:
  case MYRI_GET_COUNTERS_KB:
  case MYRI_CLEAR_COUNTERS:
  case MYRI_GET_LOGGING:
  case MYRI_GET_SRAM_SIZE:

#if MAL_OS_UDRV
  case MYRI_UDRV_DOORBELL:
#endif


  case MX_PIN_SEND:
  case MX_GET_MAPPER_MSGBUF_SIZE:
  case MX_GET_MAPPER_MAPBUF_SIZE:
  case MX_GET_MAPPER_MSGBUF:
  case MX_GET_MAPPER_MAPBUF:
  case MX_GET_PEER_FORMAT:
  case MX_GET_ROUTE_SIZE:
  case MX_GET_PEER_TABLE:
  case MX_GET_ROUTE_TABLE:
  case MX_PAUSE_MAPPER:
  case MX_RESUME_MAPPER:
  case MYRI_GET_ENDPT_OPENER:
  case MYRI_MMAP:
  case MX_CLEAR_PEER_NAMES:
  case MX_SET_PEER_NAME:
  case MX_SET_HOSTNAME:
  case MX_GET_CACHELINE_SIZE:
  case MX_GET_LINK_STATE:
  case MX_PIN_RECV:
  case MX_RUN_DMABENCH:
  case MX_CLEAR_WAIT:
  case MYRI_RAW_GET_PARAMS:
  case MX_CLEAR_ROUTES:
  case MX_GET_CPU_FREQ:
  case MX_GET_PCI_FREQ:
  case MX_GET_MAPPER_STATE:
  case MX_RECOVER_ENDPOINT:
  case MX_REMOVE_PEER:
  case MX_DIRECT_GET:
  case MX_GET_PART_NUMBER:
  case MX_WAIT_FOR_RECOVERY:
  case MYRI_PCI_CFG_READ:
  case MYRI_PCI_CFG_WRITE:
  case MX_IOCTL_ND_COMPLETE_OVERLAPPED:
  case MYRI_SNF_SET_ENDPOINT_TX:
  case MYRI_SNF_SET_ENDPOINT_RX:
  case MYRI_SNF_SET_ENDPOINT_RX_RING:
  case MYRI_SNF_RX_START:
  case MYRI_SNF_RX_STOP:
  case MYRI_SNF_RX_RING_PARAMS:
  case MYRI_SNF_RX_NOTIFY:
  case MYRI_SNF_STATS:
  case MYRI_SNF_WAIT:
    VALGRIND_PRINTF("Unhandled POST IOCTL %x\n", cmd);
  default:
    VALGRIND_PRINTF("Unknown POST IOCTL %x\n", cmd);
  }
}

#else /* MAL_VALGRIND && !defined MAL_KERNEL */

#define MAL_VALGRIND_MEMORY_MAKE_NOACCESS(p, s)
#define MAL_VALGRIND_MEMORY_MAKE_WRITABLE(p, s)
#define MAL_VALGRIND_MEMORY_MAKE_READABLE(p, s)
#define MAL_VALGRIND_MEMORY_MAKE_SEGMENTS_READABLE(s, c, l)
#define MAL_VALGRIND_PRE_IOCTL_CHECK(c, b)
#define MAL_VALGRIND_POST_IOCTL_CHECK(c, b)

#endif /* !MAL_VALGRIND || defined MAL_KERNEL */

#endif /* _mal_valgrind_h_ */
