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
 * Copyright 2005 - 2010 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#define MYRI_RAW_MAXROUTE	MCP_ROUTE_MAX_LENGTH

void myri_kraw_intr(mx_instance_state_t *is);
int  myri_kraw_next_event(mx_endpt_state_t *es, myri_raw_next_event_t *e);
int  myri_kraw_send(mx_endpt_state_t *es, myri_raw_send_t *s);
int  myri_kraw_init(mx_instance_state_t *is);
void myri_kraw_destroy(mx_instance_state_t *is);
void myri_kraw_tx_wait_pending(mx_instance_state_t *is);
