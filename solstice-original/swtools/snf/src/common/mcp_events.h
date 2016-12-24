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
 * Copyright 2003 - 2009 by Myricom, Inc.  All rights reserved.		 *
 *************************************************************************/

#ifndef _mcp_events_h_
#define _mcp_events_h_

#define MCP_RECV_TRUC_MAX_SIZE		51
#define MCP_RECV_CONNECT_MAX_SIZE	47
#define MCP_RECV_TINY_MAX_SIZE		39

#define MCP_DONE_MANY_WORDS	2


typedef enum {
  MCP_UEVT_NONE = 0,
  MCP_UEVT_DONE,
  MCP_UEVT_RECV_TRUC,
  MCP_UEVT_RECV_CONNECT,
  MCP_UEVT_RECV_TINY,
  MCP_UEVT_RECV_SMALL,
  MCP_UEVT_RECV_MEDIUM,
  MCP_UEVT_RECV_RNDV,
  MCP_UEVT_RECV_NOTIFY,
  MCP_UEVT_RECV_NACK,
  MCP_UEVT_PIO_BENCH,
  MCP_UEVT_ERROR,
  MCP_UEVT_DONE_MANY,
} mcp_uevt_type_t;

typedef enum {
  MCP_UEVT_STATUS_SUCCESS = 0,
  MCP_UEVT_STATUS_BAD_ENDPOINT,
  MCP_UEVT_STATUS_ENDPOINT_CLOSED,
  MCP_UEVT_STATUS_BAD_SESSION,
  MCP_UEVT_STATUS_BAD_RDMAWIN,
} mcp_uevt_status_t;


/* 4 Bytes */
typedef struct
{
  uint16_t lib_cookie;
  uint8_t statis;
  uint8_t type;
} mcp_uevt_done_t;

/* 16 bytes */
typedef struct
{
  uint32_t done_mask[MCP_DONE_MANY_WORDS];
  uint8_t pad[7];
  uint8_t type;
} mcp_uevt_done_many_t;

/* 64 Bytes */
typedef struct
{
  uint16_t pad0;
  uint8_t src_endpt;
  uint8_t src_generation;
  uint8_t length;
  uint8_t pad1;
  uint16_t src_peer_index;
  uint32_t session;
  uint8_t data[MCP_RECV_TRUC_MAX_SIZE];
  uint8_t type;
} mcp_uevt_truc_t;

/* 64 Bytes */
typedef struct
{
  uint16_t pad0;
  uint8_t src_endpt;
  uint8_t src_generation;
  uint8_t length;
  uint8_t pad1;
  uint16_t lib_seqnum;
  uint8_t pad2[6];
  uint16_t src_peer_index;
  uint8_t data[MCP_RECV_CONNECT_MAX_SIZE];
  uint8_t type;
} mcp_uevt_connect_t;

/* 24 Bytes */
typedef struct
{
  uint16_t pad0;
  uint8_t src_endpt;
  uint8_t src_generation;
  uint16_t length;
  uint16_t src_peer_index;
  uint16_t lib_seqnum;
  uint16_t lib_piggyack;
  uint32_t match_a;
  uint32_t match_b;
  uint32_t pad1;
} mcp_uevt_msg_t;

/* 64 Bytes */
typedef struct
{
  uint16_t pad0;
  uint8_t src_endpt;
  uint8_t src_generation;
  uint16_t length;
  uint16_t src_peer_index;
  uint16_t lib_seqnum;
  uint16_t lib_piggyack;
  uint32_t match_a;
  uint32_t match_b;
  uint32_t pad1;
  uint8_t data[MCP_RECV_TINY_MAX_SIZE];
  uint8_t type;
} mcp_uevt_tiny_t;

/* 24 Bytes */
typedef struct
{
  uint16_t pad0;
  uint8_t src_endpt;
  uint8_t src_generation;
  uint16_t length;
  uint16_t src_peer_index;
  uint16_t lib_seqnum;
  uint16_t lib_piggyack;
  uint32_t match_a;
  uint32_t match_b;
  uint16_t recvq_vpage_index;
  uint8_t pad2;
  uint8_t type;
} mcp_uevt_small_t;

/* 24 Bytes */
typedef struct
{
  uint16_t pad0;
  uint8_t src_endpt;
  uint8_t src_generation;
  uint16_t length;
  uint16_t src_peer_index;
  uint16_t lib_seqnum;
  uint16_t lib_piggyack;
  uint32_t match_a;
  uint32_t match_b;
  uint16_t recvq_vpage_index;
  uint8_t pad1;
  uint8_t type;
} mcp_uevt_medium_t;

/* 24 Bytes */
typedef struct
{
  uint16_t pad0;
  uint8_t src_endpt;
  uint8_t src_generation;
  uint32_t pad1;
  uint32_t length;
  uint8_t rdmawin_id;
  uint8_t rdmawin_seqnum;
  uint16_t pad2;
  uint16_t src_peer_index;
  uint16_t lib_seqnum;
  uint16_t lib_piggyack;
  uint8_t pad3;
  uint8_t type;
} mcp_uevt_notify_t;

/* 16 Bytes */
typedef struct
{
  uint32_t pad0[2];
  uint8_t statis;
  uint8_t dest_endpt;
  uint16_t dest_peer_index;
  uint16_t lib_seqnum;
  uint8_t pad1;
  uint8_t type;
} mcp_uevt_nack_t;

/* 64 Bytes */
typedef struct
{
  uint8_t type;
} mcp_uevt_piobench_t;


/* 64 Bytes */
typedef union
{
  struct {
    uint8_t pad[64 - sizeof (mcp_uevt_done_t)];
    mcp_uevt_done_t uevt;
  } done;
  struct {
    uint8_t pad[64 - sizeof (mcp_uevt_done_many_t)];
    mcp_uevt_done_many_t uevt;
  } done_many;
  mcp_uevt_truc_t truc;
  mcp_uevt_connect_t connect;
  mcp_uevt_tiny_t tiny;
  struct {
    uint8_t pad[64 - sizeof (mcp_uevt_small_t)];
    mcp_uevt_small_t uevt;
  } small;
  struct {
    uint8_t pad[64 - sizeof (mcp_uevt_medium_t)];
    mcp_uevt_medium_t uevt;
  } medium;
  struct {
    uint8_t pad[64 - sizeof (mcp_uevt_notify_t)];
    mcp_uevt_notify_t uevt;
  } notify;
  struct {
    uint8_t pad[64 - sizeof (mcp_uevt_nack_t)];
    mcp_uevt_nack_t uevt;
  } nack;
  struct {
    uint8_t pad[64 - sizeof (mcp_uevt_piobench_t)];
    mcp_uevt_piobench_t uevt;
  } piobench;
  struct {
    uint8_t pad[63];
    uint8_t type;
  } basic;
} mcp_uevt_t;


#endif  /* _mcp_events_h_ */
