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

#ifndef _mcp_wrapper_common_h_
#define _mcp_wrapper_common_h_

#include "mal.h"
#include "mx_pio.h"
#include "mcp_types.h"

/*
 * We use the last 16 bytes in the counters page to store the hwclock updates
 * from Lanai
 */
#define MX_MAX_COUNTER_SIZE (MX_VPAGE_SIZE-sizeof(mcp_hwclock_update_t))

static inline 
uint32_t
mcp_wrapper_getval(uint32_t *ptr, uint32_t size)
{
  mal_always_assert(size == 4);
  return ntohl(MX_PIO_READ(ptr));
}

#define MCP_GETPTR(is, field)						\
  &(((mcp_public_global_t *)(is->lanai.sram + MCP_GLOBAL_OFFSET))->field)

#define MCP_GETVAL(is, field)						\
  mcp_wrapper_getval((uint32_t *) MCP_GETPTR(is, field),		\
			sizeof(*(MCP_GETPTR(is, field))))

#define MCP_SETVAL(is, field, val)					\
  do {									\
    mal_always_assert(sizeof(*(MCP_GETPTR(is, field))) == 4);		\
    MX_PIO_WRITE(MCP_GETPTR(is, field), ntohl(val));			\
    MAL_STBAR();							\
  } while (0)


typedef enum {
  MCP_COUNTER_BAD_PKT,
  MCP_COUNTER_NIC_OVERFLOW,
  MCP_COUNTER_NIC_RECV_KBYTES,
  MCP_COUNTER_NIC_SEND_KBYTES,
  MCP_COUNTER_SNF_RECV,
  MCP_COUNTER_SNF_OVERFLOW,
  MCP_COUNTER_SNF_SEND,
} mcp_counter_name_t;


#endif /* _mcp_wrapper_common_h_ */
