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

#ifndef _counters_h_
#define _counters_h_

#include "mal_auto_config.h"


#if defined(COUNTERS_VAL)

#define COUNTERS_DEF(id) _COUNTERS_DEF(id)
#define _COUNTERS_DEF(id) struct mcp_counters_##id
#define COUNTER(field, label) uint32_t field;
#if MCP_2XP
#define COUNTER_PORT(field, label) uint32_t field[2];
#else
#define COUNTER_PORT(field, label) uint32_t field[1];
#endif

#elif defined(COUNTERS_STR)

#define COUNTERS_DEF(id) _COUNTERS_DEF(id)
#define _COUNTERS_DEF(id) const char *mcp_counters_##id##_str[] = 
#define COUNTER(field, label) label,
#if MCP_2XP
#define COUNTER_PORT(field, label) label " (Port 0)", label " (Port 1)", 
#else
#define COUNTER_PORT(field, label) label " (Port 0)",
#endif

#else /* MCP */

#define COUNTERS_DEF(id) struct mcp_counters
#define COUNTER(field, label) uint32_t field;
#if MCP_2XP
#define COUNTER_PORT(field, label) uint32_t field[2];
#else
#define COUNTER_PORT(field, label) uint32_t field[1];
#endif

#endif /* MCP */



COUNTERS_DEF(COUNTERS_ID)
{
  /* these 4 counters must not move */
  COUNTER(lanai_uptime, "Lanai uptime (seconds)")
  COUNTER(counters_uptime, "Counters uptime (seconds)")
  COUNTER_PORT(net_send_kbytes, "Net send KBytes")
  COUNTER_PORT(net_recv_kbytes, "Net recv KBytes")
  
#if MCP_MYRINET
  COUNTER(net_send_ether_unicast, "Ethernet Unicast send")
  COUNTER(net_send_ether_multicast, "Ethernet Multicast send")
#else
  COUNTER(net_send_ether_native, "Ethernet send")
#endif
  COUNTER(ether_recv_small, "Ethernet Small recv")
  COUNTER(ether_recv_big, "Ethernet Big recv")
  COUNTER(ether_down, "Ethernet recv down")
  COUNTER(ether_overrun, "Ethernet recv overrun")
  COUNTER(ether_rerecv, "Ethernet re-recv")
  COUNTER(ether_oversized, "Ethernet recv oversized")


  COUNTER(snf_send, "SNF send pkts")
  COUNTER(snf_recv, "SNF recv pkts")
  COUNTER(snf_drop_prefetch, "SNF drop prefetch race")
  COUNTER(snf_drop_flow, "SNF drop ring full")
#if MAL_DEBUG
  COUNTER(snf_credits, "SNF credits")
#endif


  COUNTER(net_send_raw, "Net send Raw")
  

  COUNTER(intr_msi, "Interrupts MSI")
  COUNTER(intr_legacy, "Interrupts Legacy")
  COUNTER(wake_intr, "Wake interrupt")
  COUNTER(wake_race, "Wake race")
  COUNTER(wake_closed, "Wake endpoint closed")
  COUNTER(net_send_queued, "Net send queued")
  COUNTER(eventq_full, "Event Queue full")
  COUNTER(rx_dataq_race, "RX DataQ race")
  COUNTER(fragmented_request, "Fragmented request")

  COUNTER_PORT(net_bad_crc32, "Net bad PHY/CRC32 drop")
#if MCP_MYRINET
  COUNTER_PORT(net_bad_crc8, "Net bad CRC8 drop")
  COUNTER(net_mapper_drop, "Net mapper drop")
#endif
  COUNTER_PORT(net_overflow_drop, "Net overflow drop")
#if MCP_2G
  COUNTER_PORT(buffer_drop, "buffer_drop")
  COUNTER_PORT(memory_drop, "memory_drop")
  COUNTER_PORT(hw_flow_control, "Hardware flow control")
  COUNTER(dma_slab_starvation, "DMA slab starvation")
#else
  COUNTER(net_ether_pause, "Net Recv PAUSEs")
  COUNTER(net_recv_alt, "Net recv alternate channel")
  COUNTER(net_alt_drop, "Net Alt drop")
#endif

  COUNTER(ether_filter_multicast, "Ethernet Multicast filter drop")
  COUNTER(ether_filter_unicast, "Ethernet Unicast filter drop")

    COUNTER(out_of_send_handles, "Out of send handles")
  COUNTER(ureq_type_unknown, "User request type unknown")
  COUNTER(spurious_request, "Spurious user request")

#if MCP_MYRINET
  COUNTER(net_send_probe, "Net send Probe")
  COUNTER(net_send_probe_ack, "Net send Probe Ack")
  COUNTER(net_send_probe_nack, "Net send Probe Nack")
  COUNTER(net_recv_probe_nack, "Net recv Probe Nack")
  COUNTER(route_dispersion, "Route dispersion")

  COUNTER(route_update, "Route Table update")
#endif

#if MAL_DEBUG
  COUNTER(devel_pkt_loss, "Packet loss (Devel !)")

  COUNTER(foo1, "Foo1")
  COUNTER(foo2, "Foo2")
  COUNTER(foo3, "Foo3")
#endif

};

#endif /* _counters_h_ */
