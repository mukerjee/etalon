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

#ifndef _mx_peer_h_
#define _mx_peer_h_

#include "mx_instance.h"

typedef struct
{
  uint16_t *peer_index_array;
  uint32_t cnt;
  uint32_t malloc_cnt;
} mx_mag_t;

#define MX_PEER_INVALID 0xffffU
#define MX_MIN_PEER 0

extern int mx_peer_hash_size;
extern int mx_biggest_peer;
extern mx_peer_t *mx_peer_table;
extern mx_mag_t *mx_mag_table;
extern mx_peer_hash_t *mx_peer_hash;
extern mx_peer_hash_t *mx_peer_overflow;
extern mx_spinlock_t mx_peer_spinlock;

int mx_init_peers(void);
void mx_destroy_peers(void);
mx_peer_hash_t * mx_peer_lookup(uint16_t mac_high16, uint32_t mac_low32);
mx_peer_hash_t * mx_peer_lookup_eth(uint16_t mac_high16, uint32_t mac_low32, int create);
int mx_peer_from_hostname(mx_peer_t *peer);
int mx_add_peer(int *hash_index, uint16_t mac_high16, uint32_t mac_low32);
void mx_add_peers(mx_instance_state_t *is);
void mx_set_hostname(mx_instance_state_t *is, char *username);
void mx_query_peer(mx_instance_state_t *is, int peer);
void mx_name_peer(mx_instance_state_t *is, int peer);
void mx_clear_peer_names(void);
int  mx_peer_remove(uint16_t mac_high16, uint32_t mac_low32);
void mx_update_peer_type(uint32_t peer_type, uint32_t peer_index, int force);
int mx_update_peer_mag_id(uint16_t peer_index, uint32_t new_mag_id);
uint16_t mx_gw_lookup(uint16_t dst_high16, uint32_t dst_low32, uint16_t src_low16);


int  myri_set_route_begin(mx_endpt_state_t *es);
int  myri_set_route_end(mx_endpt_state_t *es);
int  myri_set_route(mx_endpt_state_t *es, mx_set_route_t *r, int clear);
int  myri_update_board_routes(mx_instance_state_t *is);



#endif /* _mx_peer_h_ */
