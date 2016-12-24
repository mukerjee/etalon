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
 * Copyright 2008 by Myricom, Inc.  All rights reserved.                 *
 *************************************************************************/

#ifndef _myri_ptp_common_h_
#define _myri_ptp_common_h_

#include "mx_instance.h"
#include "bsd/queue.h"

#if MYRI_ENABLE_PTP

#define MYRI_ETH_HLEN		(14)

#define MYRI_PTP_MAX_MSG_LEN	(512) /* including Ethernet header if PTPoE */
#define MYRI_PTP_NUM_MSGS	(128)

/* PTP message to receive */
struct myri_ptp_msg {
  STAILQ_ENTRY(myri_ptp_msg) entries;
  int raw;			/* 0 or 1 - raw msgs include the Ethernet header */
  uint64_t timestamp_ns;	/* host timestamp in nanoseconds */
  uint16_t length;		/* length of msg including Ethernet header if raw */
  char *buf;			/* pointer to PTP message */
};

STAILQ_HEAD(myri_ptp_msg_head, myri_ptp_msg);

/* PTP tx timestamp */
struct myri_ptp_timestamp {
  STAILQ_ENTRY(myri_ptp_timestamp) entries;
  uint16_t ptp_type;
  uint16_t seq_id;
  uint64_t timestamp_ns;
};

STAILQ_HEAD(myri_ptp_timestamp_head, myri_ptp_timestamp);

struct myri_ptp 
{
  char *buffers;				/* space for incoming msgs */

  int16_t rx_raw_msgs_used;			/* how many rx raw ptp_msg are in-use - debugging */
  struct myri_ptp_msg *rx_raw_msgs;		/* array of rx raw msg entries */
  struct myri_ptp_msg_head idle_rx_raw_msgs;	/* unused rx raw msg entries */
  struct myri_ptp_msg_head rx_raw_msgq;		/* rx raw msgs awaiting a consumer */

  int16_t rx_msgs_used;				/* how many rx ptp_msg are in-use - debugging */
  struct myri_ptp_msg *rx_msgs;			/* array of rx msg entries */
  struct myri_ptp_msg_head idle_rx_msgs;	/* unused rx msg entries */
  struct myri_ptp_msg_head rx_msgq;		/* rx msgs awaiting a consumer */

  struct myri_ptp_timestamp *tx_timestamps;	/* array of tx timestamps */
  struct myri_ptp_timestamp_head idle_tx_ts;	/* unused tx timestamps */
  struct myri_ptp_timestamp_head tx_tsq;	/* tx timestamps awaiting a consumer */
};

/* Caller must be holding the PTP lock */
static inline struct myri_ptp_msg *
myri_ptp_get_idle_msg(mx_instance_state_t *is, int raw)
{
  struct myri_ptp *ptp = is->ptp;
  struct myri_ptp_msg *msg = NULL;
  struct myri_ptp_msg_head *idle = &ptp->idle_rx_msgs;
  int16_t *used = &ptp->rx_msgs_used;

  if (raw) {
    idle = &ptp->idle_rx_raw_msgs;
    used = &ptp->rx_raw_msgs_used;
  }

  if (is->ptp_state >= MYRI_PTP_STARTING && !STAILQ_EMPTY(idle)) {
    msg = STAILQ_FIRST(idle);
    STAILQ_REMOVE_HEAD(idle, entries);
    *used += 1;
    mal_assert(*used <= MYRI_PTP_NUM_MSGS);
  }
  return msg;
}

/* caller must be holding is->ptp_state_lock */
static inline void
myri_ptp_put_idle_msg_locked(struct myri_ptp *ptp, struct myri_ptp_msg *msg)
{
  struct myri_ptp_msg_head *idle = &ptp->idle_rx_msgs;
  int16_t *used = &ptp->rx_msgs_used;

  if (msg->raw) {
    idle = &ptp->idle_rx_raw_msgs;
    used = &ptp->rx_raw_msgs_used;
  }

  msg->length = 0;

  STAILQ_INSERT_HEAD(idle, msg, entries);
  *used -= 1;
  mal_assert(*used >= 0);

  return;
}

static inline void
myri_ptp_put_idle_msg(mx_instance_state_t *is, struct myri_ptp_msg *msg)
{
  unsigned long flags;

  flags = 0; /* -Wunused on non-linux */

  mx_spin_lock_irqsave(&is->ptp_state_lock, flags);
  myri_ptp_put_idle_msg_locked(is->ptp, msg);
  mx_spin_unlock_irqrestore(&is->ptp_state_lock, flags);

  return;
}

static inline struct myri_ptp_timestamp *
myri_ptp_get_idle_timestamp(mx_instance_state_t *is)
{
  struct myri_ptp *ptp = is->ptp;
  struct myri_ptp_timestamp *timestamp = NULL;
  struct myri_ptp_timestamp_head *idle = &ptp->idle_tx_ts;

  if (is->ptp_state >= MYRI_PTP_STARTING && !STAILQ_EMPTY(idle)) {
    timestamp = STAILQ_FIRST(idle);
    STAILQ_REMOVE_HEAD(idle, entries);
  }
  return timestamp;
}

static inline void
myri_ptp_put_idle_timestamp(struct myri_ptp *ptp, struct myri_ptp_timestamp *timestamp)
{
  struct myri_ptp_timestamp_head *idle = &ptp->idle_tx_ts;
  STAILQ_INSERT_HEAD(idle, timestamp, entries);
  return;
}

/* convert NIC ticks to host nanoseconds */
static inline uint64_t
myri_ptp_rx_nticks_to_nsecs(mx_instance_state_t *is, mcp_ether_rx_desc_t *desc)
{
  uint64_t nic_ticks, nsecs;

  nic_ticks = ((uint64_t) ntohl(desc->nticks_high) << 32) +
              ((uint64_t) ntohl(desc->nticks_low));

  nsecs = myri_clksync_nticks_update(&is->clk_nticks, nic_ticks);

  /* Convert to host time */
  return myri_clksync_convert_nsecs(&is->kernel_window->clksync, nsecs);
}

/* convert NIC ticks to host nanoseconds */
static inline uint64_t
myri_ptp_tx_nticks_to_nsecs(mx_instance_state_t *is, mcp_raw_desc_t *desc)
{
  uint64_t nic_ticks, nsecs;

  nic_ticks = ((uint64_t) ntohl(desc->nticks_high) << 32) +
              ((uint64_t) ntohl(desc->nticks_low));

  nsecs = myri_clksync_nticks_update(&is->clk_nticks, nic_ticks);

  /* Convert to host time */
  return myri_clksync_convert_nsecs(&is->kernel_window->clksync, nsecs);
}

/* specify wire formats for Ethernet, VLAN, IP, UDP, and PTP for all OSes */

#define ETH_ADDR_LEN 6

#if MAL_OS_WINDOWS
#define ATTR_PACKED
#else
#define ATTR_PACKED __attribute__ ((__packed__))
#endif

#if MAL_OS_WINDOWS
#pragma pack(push, begin_packed, 1)
#endif

/* Ethernet header wire format */
struct ATTR_PACKED myri_eth_header {
  uint8_t  eh_dst[ETH_ADDR_LEN];
  uint8_t  eh_src[ETH_ADDR_LEN];
  uint16_t eh_type; /* big endian */
};

/* VLAN header wire format */
struct ATTR_PACKED myri_vlan_header {
  uint16_t vh_TCI;
  uint16_t vh_proto;
};

/* IP header wire info */
struct ATTR_PACKED myri_ip_header {
#if MAL_CPU_BIGENDIAN == 0
  unsigned int ih_hl:4;       /* header length */
  unsigned int ih_v:4;        /* version */
#elif MAL_CPU_BIGENDIAN == 1
  unsigned int ih_v:4;        /* version */
  unsigned int ih_hl:4;       /* header length */
#else
#error "Unknown endianess"
#endif
  uint8_t  ih_tos;            /* type of service */
  uint16_t ih_len;            /* total length */
  uint16_t ih_id;             /* identification */
  uint16_t ih_off;            /* fragment offset field */
  uint8_t  ih_ttl;            /* time to live */
  uint8_t  ih_proto;          /* protocol */
  uint16_t ih_sum;            /* checksum */
  uint32_t ih_src;            /* source address */
  uint32_t ih_dst;            /* dest address */
};

/* UDP header wire info */
struct ATTR_PACKED myri_udp_header {
  uint16_t uh_sport;          /* src port */
  uint16_t uh_dport;          /* dest port */
  uint16_t uh_len;            /* length of header and data */
  uint16_t uh_csum;           /* checksum */
};

#ifndef MYRI_ETHTYPE_PTP
#define MYRI_ETHTYPE_PTP 0x88F7
#endif
#ifndef MYRI_ETHTYPE_VLAN
#define MYRI_ETHTYPE_VLAN 0x8100
#endif
#ifndef MYRI_ETHTYPE_IP
#define MYRI_ETHTYPE_IP 0x0800
#endif

#define MYRI_PTP_EVENT_TYPE_MAX 	0x3
#define MYRI_PTP_EVENT_PORT		319
#define MYRI_PTP_GENERAL_PORT		320

/* PTP header wire format */
struct ATTR_PACKED myri_ptp_header {
  uint8_t messageType;
  uint8_t versionPTP;
  uint16_t messageLength;
  uint8_t domainNumber;
  uint8_t reserved1;
  uint16_t flagField;
  uint64_t correctionField;
  uint32_t reserved2;
  uint8_t sourcePortIdentity[10];
  uint16_t sequenceId;
  uint8_t controlField;
  uint8_t logMessageInterval;
};

#if MAL_OS_WINDOWS
#pragma pack(pop, begin_packed)
#endif

void myri_ptp_fini(mx_instance_state_t *is);
int myri_ptp_alloc(mx_instance_state_t *is);
int myri_ptp_store_rx_msg(mx_instance_state_t *is, myri_ptp_store_rx_msg_t *m, int is_kernel);
int myri_ptp_put_tx_timestamp(mx_instance_state_t *is, myri_ptp_tx_timestamp_t *x, int is_kernel);
int myri_ptp_ioctl(uint32_t cmd, const uaddr_t in, uint32_t is_kernel);

#endif /* MYRI_ENABLE_PTP */
#endif /* _myri_ptp_common_h_ */
