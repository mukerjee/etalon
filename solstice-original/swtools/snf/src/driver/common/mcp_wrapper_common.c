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
#include "mcp_wrapper_common.h"

#if MX_2G_ENABLED
#include "mcp_array_2g.c"
#endif

#if MX_10G_ENABLED
#include "mcp_array_10g.c"
#endif

#define CTA(x) do { char (*a)[(x) ? 1 : -1] = 0; (void) a; } while (0)

int
myri_select_mcp(int board_type, const unsigned char **mcp,
		int *len, unsigned int *parity_critical_start,
		unsigned int *parity_critical_end)
{
  switch (board_type) {
#if MX_2G_ENABLED
  case MAL_BOARD_TYPE_D:
    *mcp = mcp_array_d;
    *len = mcp_array_d_length;
    *parity_critical_start = mcp_d_parity_critical_start;
    *parity_critical_end = mcp_d_parity_critical_end;
    break;

  case MAL_BOARD_TYPE_E:
    *mcp = mcp_array_e;
    *len = mcp_array_e_length;
    *parity_critical_start = mcp_e_parity_critical_start;
    *parity_critical_end = mcp_e_parity_critical_end;
    break;
#endif
  
#if MX_10G_ENABLED
  case MAL_BOARD_TYPE_ZOM:
    *mcp = mcp_array_zm;
    *len = mcp_array_zm_length;
    *parity_critical_start = 0;
    *parity_critical_end = 0;
    break;

  case MAL_BOARD_TYPE_ZOE:
    *mcp = mcp_array_ze;
    *len = mcp_array_ze_length;
    *parity_critical_start = 0;
    *parity_critical_end = 0;
    break;

  case MAL_BOARD_TYPE_ZOMS:
    *mcp = mcp_array_zms;
    *len = mcp_array_zms_length;
    *parity_critical_start = 0;
    *parity_critical_end = 0;
    break;

  case MAL_BOARD_TYPE_ZOES:
    *mcp = mcp_array_zes;
    *len = mcp_array_zes_length;
    *parity_critical_start = 0;
    *parity_critical_end = 0;
    break;
#endif
  
  default:
    /* we should never get here */
    *mcp = NULL;
    *len = 0;
    break;
  }

  if (*len == 0)
    return 1;
  
  return 0;
}


int
myri_counters(int board_type, const char ***counters, uint32_t *count)
{
  switch (board_type) {
#if MX_2G_ENABLED
  case MAL_BOARD_TYPE_D:
    *counters = mcp_counters_d_str;
    *count = sizeof(struct mcp_counters_d) / sizeof(uint32_t);
    CTA(sizeof(struct mcp_counters_d) <= MX_MAX_COUNTER_SIZE);
    break;

  case MAL_BOARD_TYPE_E:
    *counters = mcp_counters_e_str;
    *count = sizeof(struct mcp_counters_e) / sizeof(uint32_t);
    CTA(sizeof(struct mcp_counters_e) <= MX_MAX_COUNTER_SIZE);
    break;
#endif

#if MX_10G_ENABLED  
  case MAL_BOARD_TYPE_ZOM:
  case MAL_BOARD_TYPE_ZOMS:
    *counters = mcp_counters_zm_str;
    *count = sizeof(struct mcp_counters_zm) / sizeof(uint32_t);
    CTA(sizeof(struct mcp_counters_zm) <= MX_MAX_COUNTER_SIZE);
    break;

  case MAL_BOARD_TYPE_ZOE:
  case MAL_BOARD_TYPE_ZOES:
    *counters = mcp_counters_ze_str;
    *count = sizeof(struct mcp_counters_ze) / sizeof(uint32_t);
    CTA(sizeof(struct mcp_counters_ze) <= MX_MAX_COUNTER_SIZE);
    break;
#endif
  
  default:
    /* we should never get here */
    return 1;
  }
  return 0;
}



#define MCP_COUNTER(is, type, field) ((type *) is->counters.addr)->field

#if MX_2G_ENABLED
#define MCP_COUNTER_D(is, field)		\
  MCP_COUNTER(is, struct mcp_counters_d, field)
#define MCP_COUNTER_E(is, field)		\
  MCP_COUNTER(is, struct mcp_counters_e, field)
#else
#define MCP_COUNTER_D(is, field) 0
#define MCP_COUNTER_E(is, field) 0
#endif

#if MX_10G_ENABLED
#define MCP_COUNTER_ZM(is, field)			\
  MCP_COUNTER(is, struct mcp_counters_zm, field)
#define MCP_COUNTER_ZE(is, field)			\
  MCP_COUNTER(is, struct mcp_counters_ze, field)
#else
#define MCP_COUNTER_ZM(is, field) 0
#define MCP_COUNTER_ZE(is, field) 0
#endif

#define MCP_GET_COUNTER(is, field)		\
  switch (is->board_type) {			\
  case MAL_BOARD_TYPE_D:			\
    return ntohl(MCP_COUNTER_D(is, field));     \
  case MAL_BOARD_TYPE_E:			\
    return ntohl(MCP_COUNTER_E(is, field));	\
  case MAL_BOARD_TYPE_ZOM:			\
  case MAL_BOARD_TYPE_ZOMS:			\
    return ntohl(MCP_COUNTER_ZM(is, field));	\
  case MAL_BOARD_TYPE_ZOE:			\
  case MAL_BOARD_TYPE_ZOES:			\
    return ntohl(MCP_COUNTER_ZE(is, field));	\
  default:					\
    /* we should never get here */		\
    return 0;					\
  }

uint32_t
myri_get_counter(mx_instance_state_t *is, mcp_counter_name_t counter)
{
  switch (counter) {
  case MCP_COUNTER_BAD_PKT:
    MCP_GET_COUNTER(is, net_bad_crc32[0]);
    
  case MCP_COUNTER_NIC_OVERFLOW:
    MCP_GET_COUNTER(is, net_overflow_drop[0]);
    
  case MCP_COUNTER_NIC_SEND_KBYTES:
    MCP_GET_COUNTER(is, net_send_kbytes[0]);
    
  case MCP_COUNTER_NIC_RECV_KBYTES:
    MCP_GET_COUNTER(is, net_recv_kbytes[0]);

  case MCP_COUNTER_SNF_RECV:
    MCP_GET_COUNTER(is, snf_recv);
 
  case MCP_COUNTER_SNF_OVERFLOW:
    MCP_GET_COUNTER(is, snf_drop_flow);

  case MCP_COUNTER_SNF_SEND:
    MCP_GET_COUNTER(is, snf_send);
  
  default:
    /* we should never get here */
    MX_WARN(("Fail to get unknown counter (%d)\n", (int) counter));
    return 0;
  }
}
