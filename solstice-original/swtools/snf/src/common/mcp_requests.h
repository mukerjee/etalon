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

#ifndef _mcp_requests_h_
#define _mcp_requests_h_

#define MCP_ETHER_FLAGS_VALID		0x1
#define MCP_ETHER_FLAGS_LAST		0x2
#define MCP_ETHER_FLAGS_HEAD		0x4
#define MCP_ETHER_FLAGS_CKSUM		0x8
#define MCP_ETHER_FLAGS_GATEWAY		0x10

#define MCP_ETHER_MAX_PSEUDO_OFF	128

#define MCP_SEND_TRUC_MAX_SIZE		51
#define MCP_SEND_CONNECT_MAX_SIZE	47
#define MCP_SEND_TINY_MAX_SIZE		39
#define MCP_PUT_TINY_MAX_SIZE		36


typedef enum {
  MCP_UREQ_NONE = 0,
  MCP_UREQ_SEND_TRUC,
  MCP_UREQ_SEND_CONNECT,
  MCP_UREQ_SEND_TINY,
  MCP_UREQ_SEND_SMALL,
  MCP_UREQ_SEND_MEDIUM,
  MCP_UREQ_SEND_RNDV,
  MCP_UREQ_SEND_NOTIFY,
  MCP_UREQ_PULL,
  MCP_UREQ_WAKE,
  MCP_UREQ_PIOBENCH,
  MCP_UREQ_PUT_TINY,
  MCP_UREQ_PUT_LARGE,
  MCP_UREQ_LAST_TYPE
} mcp_ureq_type_t;

/* Be cache friendly */
#define MCP_UREQ_ALIGNMENT	64

/* 64 Bytes */
typedef struct
{
  uint8_t pad0;
  uint8_t dest_endpt;
  uint16_t dest_peer_index;
  uint8_t length;
  uint8_t pad1;
  uint16_t lib_cookie;
  uint32_t session;
  uint8_t data[MCP_SEND_TRUC_MAX_SIZE];
  uint8_t type;
} mcp_ureq_truc_t;

/* 64 Bytes */
typedef struct
{
  uint8_t pad0;
  uint8_t dest_endpt;
  uint16_t pad1;
  uint8_t length;
  uint8_t pad2;
  uint16_t lib_cookie;
  uint16_t lib_seqnum;
  uint16_t dest_peer_index;
  uint32_t pad3;
  uint8_t data[MCP_SEND_CONNECT_MAX_SIZE];
  uint8_t type;
} mcp_ureq_connect_t;

/* 64 Bytes */
typedef struct
{
  uint8_t pad0;
  uint8_t dest_endpt;
  uint16_t dest_peer_index;
  uint16_t length;
  uint16_t lib_cookie;
  uint16_t lib_seqnum;
  uint16_t lib_piggyack;
  uint32_t match_a;
  uint32_t match_b;
  uint32_t session;
  uint8_t data[MCP_SEND_TINY_MAX_SIZE];
  uint8_t type;
} mcp_ureq_tiny_t;

/* 64 Bytes */
typedef struct
{
  uint8_t pad0;
  uint8_t dest_endpt;
  uint16_t dest_peer_index;
  uint16_t length;
  uint16_t lib_cookie;
  uint16_t lib_seqnum;
  uint16_t lib_piggyack;
  uint32_t match_a;
  uint32_t match_b;
  uint32_t session;
  uint16_t offset;
  uint8_t pad1[37];
  uint8_t type;
} mcp_ureq_small_t;

/* 64 Bytes */
typedef struct
{
  uint8_t pad0;
  uint8_t dest_endpt;
  uint16_t dest_peer_index;
  uint16_t length;
  uint16_t lib_cookie;
  uint16_t lib_seqnum;
  uint16_t lib_piggyack;
  uint32_t match_a;
  uint32_t match_b;
  uint32_t session;
  uint16_t sendq_index;
  uint8_t pad1[37];
  uint8_t type;
} mcp_ureq_medium_t;

/* 64 Bytes */
typedef struct
{
  uint8_t pad0;
  uint8_t dest_endpt;
  uint16_t dest_peer_index;
  uint32_t session;
  uint32_t length;
  uint8_t target_rdmawin_id;
  uint8_t target_rdmawin_seqnum;
  uint16_t target_rdma_offset;
  uint8_t origin_rdmawin_id;
  uint8_t origin_rdmawin_seqnum;
  uint16_t origin_rdma_offset;
  uint16_t lib_cookie;
  uint8_t pad1[41];
  uint8_t type;
} mcp_ureq_pull_t;

/* 64 Bytes */
typedef struct
{
  uint8_t pad0;
  uint8_t dest_endpt;
  uint16_t dest_peer_index;
  uint32_t session;
  uint32_t length;
  uint8_t target_rdmawin_id;
  uint8_t target_rdmawin_seqnum;
  uint16_t pad1;
  uint16_t lib_cookie;
  uint16_t lib_seqnum;
  uint16_t lib_piggyack;
  uint8_t pad2[41];
  uint8_t type;
} mcp_ureq_notify_t;

/* 64 Bytes */
typedef struct
{
  uint8_t pad0;
  uint8_t dest_endpt;
  uint16_t dest_peer_index;
  uint32_t session;
  uint32_t remote_virt_high;
  uint32_t remote_virt_low;
  uint32_t length;
  uint16_t seqnum;
  uint16_t pad;
  union {
    struct {
      uint8_t data[MCP_PUT_TINY_MAX_SIZE];
    } pio;
    struct {
      uint32_t pad_large[7];
      uint32_t virt_high;
      uint32_t virt_low;
    } dma;
  } local;
  uint16_t lib_cookie;
  uint8_t pad2;
  uint8_t type;
} mcp_ureq_put_t;

/* 64 Bytes */
typedef struct
{
  uint32_t eventq_flow;
  uint8_t pad[59];
  uint8_t type;
} mcp_ureq_wake_t;

/* 64 Bytes */
typedef struct
{
  uint8_t pad[62];
  uint8_t length;
  uint8_t type;
} mcp_ureq_piobench_t;


/* 64 Bytes */
typedef union
{
  mcp_ureq_truc_t truc;
  mcp_ureq_connect_t connect;
  mcp_ureq_tiny_t tiny;
  mcp_ureq_small_t small;
  mcp_ureq_medium_t medium;
  mcp_ureq_notify_t notify;
  mcp_ureq_pull_t pull;
  mcp_ureq_put_t put;
  mcp_ureq_wake_t wake;
  mcp_ureq_piobench_t piobench;
  struct {
    uint8_t pad[63];
    uint8_t type;
  } basic;
  uint64_t int_array[64/8];
} mcp_ureq_t;



typedef enum {
  MCP_KREQ_NONE = 0,
  MCP_KREQ_RAW,
  MCP_KREQ_COMMAND,
  MCP_KREQ_QUERY,
} mcp_kreq_type_t;


/* 24 Bytes */
typedef struct
{
  uint32_t addr_high;
  uint32_t addr_low;
  uint32_t data_len;
  uint8_t route[8];
  uint8_t route_len;
  uint8_t port;
  uint8_t tx_id;
  uint8_t type;
} mcp_kreq_raw_t;

/* 16 Bytes */
typedef struct
{
  uint32_t index;
  uint32_t data0;
  uint32_t data1;
  uint16_t pad;
  uint8_t cmd;
  uint8_t type;
} mcp_kreq_command_t;

/* 4 Bytes */
typedef struct
{
  uint16_t peer_index;
  uint8_t pad;
  uint8_t type;
} mcp_kreq_query_t;

/* 32 Bytes */
typedef union
{
  struct {
    uint8_t pad[32 - sizeof (mcp_kreq_raw_t)];
    mcp_kreq_raw_t req;
  } raw;
  struct {
    uint8_t pad[32 - sizeof (mcp_kreq_command_t)];
    mcp_kreq_command_t req;
  } cmd;
  struct {
    uint8_t pad[32 - sizeof (mcp_kreq_query_t)];
    mcp_kreq_query_t req;
  } query;
  struct {
    uint8_t pad[31];
    uint8_t type;
  } basic;
  uint64_t int64_array[32/8];
} mcp_kreq_t;


/* 16 Bytes */
typedef union
{
  struct {
    uint32_t dest_low32;
    uint16_t dest_high16;
    uint16_t cksum_offset;      /* where to start computing cksum */
    uint16_t pseudo_hdr_offset; /* where to store cksum */
    uint16_t peer_index;
    uint8_t pad[3];
    uint8_t flags;
  } head;
  struct {
    uint32_t addr_high;
    uint32_t addr_low;
    uint32_t length;
    uint8_t pad[3];
    uint8_t flags;
  } frag;
} mcp_kreq_ether_send_t;

#endif  /* _mcp_requests_h_ */
