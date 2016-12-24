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
#include "mx_misc.h"
#include "mx_instance.h"
#include "mx_malloc.h"
#include "mx_peer.h"
#include "mx_pio.h"
#include "myri_raw.h"
#include "kraw.h"

int mx_peer_hash_size;
int mx_biggest_peer;
int mx_peer_overflow_count;
mx_mag_t *mx_mag_table;
mx_peer_t *mx_peer_table;
mx_peer_hash_t *mx_peer_hash;
mx_peer_hash_t *mx_peer_overflow;
mx_spinlock_t mx_peer_spinlock;


int
mx_init_peers(void)
{
  if (myri_mx_max_nodes * 2 > myri_mx_max_macs)
    myri_mx_max_macs = myri_mx_max_nodes * 2;
  if (myri_mx_max_macs < 4096)
    myri_mx_max_macs = 4096;
  mx_peer_hash_size = myri_mx_max_macs * 4;

  mx_peer_table = mx_kmalloc(sizeof(mx_peer_table[0]) * myri_mx_max_nodes, 
			     MX_MZERO|MX_WAITOK);
  if (!mx_peer_table)
    goto abort;

  mx_peer_overflow = mx_kmalloc(sizeof(mx_peer_overflow[0]) * myri_mx_max_macs, 
				MX_MZERO|MX_WAITOK);
  if (!mx_peer_overflow)
    goto abort;

  mx_mag_table = mx_kmalloc(sizeof(mx_mag_table[0]) * myri_mx_max_nodes, 
			       MX_MZERO|MX_WAITOK);
  if (!mx_mag_table)
    goto abort;

  mx_peer_hash = mx_kmalloc(sizeof(mx_peer_hash[0]) * mx_peer_hash_size,
			    MX_MZERO|MX_WAITOK);

  if (!mx_peer_hash)
    goto abort;

  mx_spin_lock_init(&mx_peer_spinlock, NULL, -1, "peer spinlock");

  return 0;

 abort:
  mx_destroy_peers();

  return ENOMEM;
}

void
mx_destroy_peers()
{
  if (mx_peer_overflow)
    mx_kfree(mx_peer_overflow);
  if (mx_peer_table)
    mx_kfree(mx_peer_table);
  if (mx_mag_table) {
    int i;
    for (i=0; i < myri_mx_max_nodes; i++) {
      if (mx_mag_table[i].peer_index_array)
	mx_kfree(mx_mag_table[i].peer_index_array);
    }
    mx_kfree(mx_mag_table);
  }
  if (mx_peer_hash) {
    mx_kfree(mx_peer_hash);
    mx_spin_lock_destroy(&mx_peer_spinlock);
  }
}

static inline int
mx_peer_hash_fn(uint16_t a, uint16_t b)
{
  int val;

  val  = (a ^ b) & (mx_peer_hash_size - 1 );
  return val;
}

static mx_peer_hash_t *
mx_peer_lookup_bin(mx_peer_hash_t **freebin, uint16_t mac_high16, uint32_t mac_low32)
{
  int i, high, low;
  uint64_t key, tmp;
  unsigned index;

  /* lookup the mac address in our peer hash table */
  index = mx_peer_hash_fn((uint16_t)(mac_low32 >> 16), (uint16_t)mac_low32);
  /* incorrect call to test collision table 
    index = mx_peer_hash_fn((uint16_t)(mac_low32 >> 16), mac_high16); 
   */
  if (mx_peer_hash[index].mac_low32 == mac_low32 &&
      mx_peer_hash[index].mac_high16 == mac_high16) {
    return mx_peer_hash + index; /* match! */
  }

  /* if the address doesn't match, check to see if there
     is a collision, or if the address hasn't been seen */

  if (mx_peer_hash[index].mac_low32 == 0 &&
      mx_peer_hash[index].mac_high16 == 0) {
    *freebin = mx_peer_hash + index;
    return NULL; 
  }

  /* A collision occured, so it will be in the overflow table.  This
     table is kept sorted so we can do a binary search */

  low = 0;
  high = mx_peer_overflow_count;
  key = ((uint64_t)mac_high16 << 32) | mac_low32;
  do {
    i = (high + low) / 2;
    tmp = ((uint64_t)mx_peer_overflow[i].mac_high16 << 32) 
      | mx_peer_overflow[i].mac_low32;
    if (key == tmp) {
      /* return ESRCH to let the caller know the index pertains
	 to the overflow table, not the peer hash. */
      return mx_peer_overflow + i;
    }
    if (key <= tmp || tmp == 0)
      high = i - 1;
    else
      low = i + 1;
  } while (low <= high);

  /* we get here if the there was a collision, but the address
     isn't already in the overflow table */
  *freebin = NULL;
  return NULL;
}

mx_peer_hash_t *
mx_peer_lookup(uint16_t mac_high16, uint32_t mac_low32)
{
  mx_peer_hash_t *free_bin, *bin;
  
  bin = mx_peer_lookup_bin(&free_bin, mac_high16, mac_low32);
  if (bin && bin->index == MX_PEER_INVALID)
    bin = NULL;
  return bin;
    
}

int
mx_peer_remove(uint16_t mac_high16, uint32_t mac_low32)
{
  int peer_idx, status = 0;
  mx_peer_hash_t *hash;
  unsigned long flags;

  flags = 0; /* defeat -Wno-unused */

  mx_spin_lock_irqsave(&mx_peer_spinlock, flags);
  hash = mx_peer_lookup(mac_high16, mac_low32);
  if (hash) {
    /* found in main hash table */
    peer_idx = hash->index;
    bzero(hash, sizeof (*hash));
    bzero(&mx_peer_table[peer_idx], sizeof (mx_peer_table[peer_idx]));
  } else {
    status = ESRCH;
  }

  mx_spin_unlock_irqrestore(&mx_peer_spinlock, flags);
  return status;
}


int
mx_peer_from_hostname(mx_peer_t *peer)
{
  int i;
  size_t len;
  int active_found = -1;
  int inactive_found = -1;
  len = strlen(peer->node_name) + 1;
  /* XXX locking? */
  for (i = 0; i <= mx_biggest_peer; i++) {
    if (!strncmp(peer->node_name, mx_peer_table[i].node_name, len)) {
      if (mx_peer_table[i].flags & MX_PEER_FLAG_SEEN_ANY)
	active_found = i;
      else if (!(mx_peer_table[i].flags & MX_PEER_FLAG_SEEN_ANY))
	inactive_found = i;
    }
  }
  if ((i = active_found) != -1 || (i = inactive_found) != -1) {
      bcopy(&mx_peer_table[i], peer, sizeof(*peer));
      return 0;
  }
  return ENOENT;
}

static int
mx_peer_query_needed(mx_instance_state_t *is, int peer_index)
{
  mx_peer_t *peer;
  mx_routes_t *routes0, *routes1;

  peer = mx_peer_table + peer_index;
  routes0 =  &is->routes[0];
  routes1 =  &is->routes[is->num_ports - 1]; /* last set of route */
  return peer->node_name[0] == '\0' /* skip named peers */
    && (peer->type == MX_HOST_MX || peer->type == MX_HOST_MXvM) /* skip non-MX nodes */
    && (routes0->offsets[peer_index] != 0  /* skip unreachable peers */
	|| routes1->offsets[peer_index] != 0);

}

void
mx_query_peer(mx_instance_state_t *is, int peer)
{
  mcp_kreq_t kreq;
  unsigned long flags;
  int starting_peer;

  flags = 0; /* useless initialization to pacify -Wunused on platforms
		where flags are not used */

  mx_spin_lock_irqsave(&mx_peer_spinlock, flags);
  if (peer == 0) {
    is->querying_done = 0;
    is->querying_rollover = 0;
  }
  if (is->query_pending || is->querying_done)
    goto abort_with_spinlock;


  if (peer > mx_biggest_peer) {
    is->querying_rollover += 1;
    peer = 0;
  }
  starting_peer = peer;

  while (!mx_peer_query_needed(is, peer)) {
    peer += 1;
    if (peer > mx_biggest_peer) {
      peer = 0;
      is->querying_rollover += 1;
    }
    if (peer == starting_peer) {
      is->querying_done = 1;
      goto abort_with_spinlock; /* all queries have been done */
    } else if (myri_mx_max_host_queries 
	       && is->querying_rollover >= myri_mx_max_host_queries) {
      goto abort_with_spinlock; /* stop querying */
    }
  }

  kreq.query.req.peer_index = htons((uint16_t)peer);
  kreq.query.req.type = MCP_KREQ_QUERY; /* 1 byte, no swapping */ 
  
  mx_spin_lock(&is->kreqq_spinlock);
  is->board_ops.write_kreq(is, &kreq);
  mx_spin_unlock(&is->kreqq_spinlock);
  is->query_pending = 1;

 abort_with_spinlock:
  mx_spin_unlock_irqrestore(&mx_peer_spinlock, flags);
}


void
mx_update_peer_type(uint32_t type, uint32_t peer_index, int force)
{
  unsigned long flags;

  flags = 0; /* -Wno-unused */

  if (type == mx_peer_table[peer_index].type && force == 0)
    return;

  mx_peer_table[peer_index].type = type;

  if (mx_peer_table[peer_index].flags & MX_PEER_FLAG_LOCAL)
    return;

  mx_spin_lock_irqsave(&mx_peer_spinlock, flags);

  switch(type) {
  case MX_HOST_MX:
  case MX_HOST_MXvM:
    bzero(mx_peer_table[peer_index].node_name, 
	  sizeof(mx_peer_table[peer_index].node_name));
    break;

  case MX_HOST_GM:
    strcpy(mx_peer_table[peer_index].node_name, "GM node");
    break;

  case MX_HOST_XM:
    mx_has_xm = 1;
    strcpy(mx_peer_table[peer_index].node_name, "Myrinet Ethernet Bridge");
    break;

  default:
    strcpy(mx_peer_table[peer_index].node_name, "non-MX node");
    break;
  }

  mx_spin_unlock_irqrestore(&mx_peer_spinlock, flags);
}


void
mx_add_peers(mx_instance_state_t *is)
{
  int i, status, hash_index, local_peer_index;
  uint32_t dummy, mac_high16, mac_low32;
  mx_peer_hash_t * hash;

  for (i = MX_MIN_PEER; i < myri_mx_max_nodes; i++) {
    if (mx_peer_table[i].mac_low32 == 0 &&
	mx_peer_table[i].mac_high16 == 0)
      break;
    status = mx_mcp_command(is, MCP_CMD_ADD_PEER, i,
			    mx_peer_table[i].mac_high16, 
			    mx_peer_table[i].mac_low32,  
			    &dummy);
    if (status)
      MX_WARN(("%s: Failed to set LATE peer, status = %d\n", 
	       is->is_name, status));
  }

  mac_high16 = MCP_GETVAL(is, mac_high16);
  mac_low32 = MCP_GETVAL(is, mac_low32);

  /* add ourself to the peer table */
  mx_add_peer(&hash_index, (uint16_t)mac_high16, mac_low32);

  /* find our peer index table */

  hash = mx_peer_lookup((uint16_t)mac_high16, mac_low32);
  mal_assert(hash);
  local_peer_index = hash->index;

  /* mark ourselves local */
  mx_peer_table[local_peer_index].flags |= MX_PEER_FLAG_LOCAL;

  /* Tell the mcp what his peer index is */
  MCP_SETVAL(is, local_peer_index, local_peer_index);

  /* set the hostname on this board */
  mx_set_hostname(is, 0);
  mx_update_peer_type(MX_HOST_MX, local_peer_index, 0);
}


static mx_peer_hash_t *
mx_add_peer_hash_overflow(uint16_t mac_high16, uint32_t mac_low32)
{
  uint64_t key, tmp;
  unsigned index;
  int i;

  /* Add to peer overflow table.  To keep the overflow table sorted,
     we do a simple insertion sort */
  mx_peer_overflow_count++;
  key = ((uint64_t)mac_high16 << 32) | mac_low32;
  
  /* find the place to insert */
  for (index = 0; index < myri_mx_max_macs; index++) {
    tmp = ((uint64_t)mx_peer_overflow[index].mac_high16 << 32)
      | mx_peer_overflow[index].mac_low32;
    if (key < tmp || tmp == 0)
      break;
  }

  /* find the end of the array */
  for (i = index; i < myri_mx_max_macs; i++) {
    if (mx_peer_overflow[i].mac_low32 == 0 && 
	mx_peer_overflow[i].mac_high16 == 0)
      break;
  }

  /* we should never run out of space */
  mal_assert(i < myri_mx_max_macs);

  /* shuffle elements up to make space */
  for (/*nothing*/; i > index; i--) {
    bcopy(&mx_peer_overflow[i-1], &mx_peer_overflow[i],
	  sizeof(mx_peer_overflow[0]));
  }
  
  return mx_peer_overflow + index;
}

int
mx_add_peer(int *hash_index, uint16_t mac_high16, uint32_t mac_low32)
{
  mx_instance_state_t *is;
  mx_peer_hash_t * bin, *hash;
  int lookup, ret;
  int i, peer_index, status;
  static int warned;
  unsigned long flags;
  uint32_t dummy;

  flags = 0; /* -Wno-unused */

  ret = 0;
  /* grab the peer lock to make sure the peer table
     does not change out from under us */

  mx_spin_lock_irqsave(&mx_peer_spinlock, flags);
  
  /* redo the lookup with the lock held to make sure another board
     didn't add the peer */

  hash = mx_peer_lookup_bin(&bin, mac_high16, mac_low32);
  if (hash && hash->index != MX_PEER_INVALID)
    goto abort_with_sync;

  /* find space in the peer table.  Scan linearly -- peer table
     insertions are rare.  We can afford to be inefficent */

  for (peer_index = MX_MIN_PEER; peer_index < myri_mx_max_nodes; peer_index++) {
    if (0 && mx_peer_table[peer_index].mac_low32 == mac_low32 &&
	mx_peer_table[peer_index].mac_high16 == mac_high16) {
      MX_WARN(("peer 0x%x 0x%x already in table at index %d, (%d, %d)\n", 
	       mac_high16, mac_low32, peer_index, *hash_index, lookup));
      /*panic("mx bad bad bad");*/
    }
    if (mx_peer_table[peer_index].mac_low32 == 0 &&
	mx_peer_table[peer_index].mac_high16 == 0)
      break;
  }

  if (peer_index >= myri_mx_max_nodes) {
    if (!warned) {
      MX_WARN(("Attempted to add more than the configured %d number of nodes\n"
	       "Consider increasing myri_mx_max_nodes\n", myri_mx_max_nodes));
      warned = 1;
    }
    ret = ENOSPC;
    goto abort_with_sync;
  }
  if (peer_index > mx_biggest_peer)
    mx_biggest_peer = peer_index;

  /* add entry to the peer table at index i */
  mx_peer_table[peer_index].mac_low32 = mac_low32;
  mx_peer_table[peer_index].mac_high16 = mac_high16;
  mx_peer_table[peer_index].flags = 0;

  /* add to peer hash */
  if (hash) /* indirect entry upgraded */
    bin = hash;
  if (!bin) {
    bin = mx_add_peer_hash_overflow(mac_high16, mac_low32);
  }
  bin->mac_low32 = mac_low32;
  bin->mac_high16 = mac_high16;
  bin->index = peer_index;
  bin->gw = MX_PEER_INVALID;

  mx_spin_unlock_irqrestore(&mx_peer_spinlock, flags);
  /* Tell the lanai about the new peer */
  for (i = 0; i < myri_max_instance; i++) {
    is = mx_instances[i];
    if (!is)
      continue;
    status = mx_mcp_command(is, MCP_CMD_ADD_PEER, peer_index,
			    mac_high16, mac_low32, &dummy);
    if (status) {
      MX_WARN(("%s: Failed to set peer, status = %d\n", is->is_name, status));
    }
  }
  return 0;

 abort_with_sync:
  mx_spin_unlock_irqrestore(&mx_peer_spinlock, flags);
  return ret;

}

mx_peer_hash_t *
mx_peer_lookup_eth(uint16_t mac_high16, uint32_t mac_low32, int create)
{
  mx_peer_hash_t *free_bin, *bin;
  unsigned long flags;

  flags = 0; /* defeat -Wno-unused */
  
  bin = mx_peer_lookup_bin(&free_bin, mac_high16, mac_low32);
  if (!bin && create && myri_mx_max_macs - mx_peer_overflow_count > myri_mx_max_nodes) {
    mx_spin_lock_irqsave(&mx_peer_spinlock, flags);
    bin = mx_peer_lookup_bin(&free_bin, mac_high16, mac_low32);
    if (!bin) {
      if (!free_bin)
	free_bin = mx_add_peer_hash_overflow(mac_high16, mac_low32);
      if (free_bin) {
	free_bin->mac_high16 = mac_high16;
	free_bin->mac_low32 = mac_low32;
	free_bin->index = MX_PEER_INVALID;
	free_bin->gw = MX_PEER_INVALID;
      }
      bin = free_bin;
    }
    mx_spin_unlock_irqrestore(&mx_peer_spinlock, flags);
  }
  return bin;
}

void
mx_name_peer(mx_instance_state_t *is, int peer)
{
  size_t len;
  unsigned long flags;

  flags = 0; /* useless initialization to pacify -Wunused on platforms
		where flags are not used */

  mx_spin_lock_irqsave(&mx_peer_spinlock, flags);
  /* make sure strlen never walks off a non-null terminated string */
  is->host_query.buf[MX_VPAGE_SIZE -1] = '\0';
  len = strlen(is->host_query.buf);
  if (peer > mx_biggest_peer) {
    MX_WARN(("%s: Can't name peer (%d) larger than biggest peer (%d)",
	     is->is_name, peer, mx_biggest_peer));
  } else if (len) {
    len = MIN(len, sizeof(mx_peer_table[peer].node_name) - 1);

    /* update the peer table */
    strncpy(mx_peer_table[peer].node_name, is->host_query.buf, len);

    /* null first byte of reply buffer, so that a timeout doesn't
       result in the last completed query result being duplicated */
    is->host_query.buf[0] = '\0';
  }
  is->query_pending = 0;
  mx_spin_unlock_irqrestore(&mx_peer_spinlock, flags);
  mx_query_peer(is, peer + 1);
}

void
mx_set_hostname(mx_instance_state_t *is, char *username)
{  
  char *c, *hostname, tmp[MYRI_MAX_STR_LEN];
  size_t len;
  uint32_t hostname_len, mac_high16, mac_low32;
  int peer_index;
  mx_peer_hash_t *hash;

  if (username)
    hostname = username;
  else
    hostname = mx_default_hostname;

  len = strlen(hostname);
  if (!len)
    return;
  
  /* get size & location of mcp hostname field */
  hostname_len = sizeof(*MCP_GETPTR(is, hostname));
  c = (char *)MCP_GETPTR(is, hostname);
  tmp[hostname_len - 1] = '\0';
  if (username) /* assume user knows what he is doing
		   and don't append trailing :INTERFACE_ID */
    snprintf(tmp, hostname_len - 1, "%s", hostname);
  else
    snprintf(tmp, hostname_len - 1, "%s:%d", hostname, is->id);
  mx_pio_memcpy(c, tmp, hostname_len, MX_PIO_32BYTE_FLUSH | MX_PIO_FLUSH | MX_PIO_32BIT_ALIGN);
  is->flags |= MX_HOSTNAME_SET;

  /* find index into peer table */
  mac_high16 = MCP_GETVAL(is, mac_high16);
  mac_low32 = MCP_GETVAL(is, mac_low32);
  hash = mx_peer_lookup(mac_high16, mac_low32);
  if (!hash) {
    MX_WARN(("%s: Can't find my peer idx to set name?\n", is->is_name));
    return;
  }
  peer_index = hash->index;
  
  /* update the peer table with our own name */
  c = mx_peer_table[peer_index].node_name;
  if (username) /* assume user knows what he is doing
		   and don't append trailing :INTERFACE_ID */
    snprintf(c, hostname_len - 1, "%s", hostname);
  else
    snprintf(c, hostname_len - 1, "%s:%d", hostname, is->id);
}

void
mx_clear_peer_names(void)
{
  int i;
  mx_instance_state_t *is;

  /* clear all the peer names */
  for (i = 0; i <= mx_biggest_peer; i++) {
    mx_update_peer_type(mx_peer_table[i].type, i, 1);
  }

  /* now start the queries going again on all nics */
  mx_mutex_enter(&mx_global_mutex);
  for (i = 0; i < myri_max_instance; ++i) {
    is = mx_instances[i];
    if (!is)
      continue;
    mx_query_peer(is, 0);
  }
  mx_mutex_exit(&mx_global_mutex);
}

uint16_t
mx_gw_lookup(uint16_t dst_high16, uint32_t dst_low32, uint16_t src_low16)
{
  mx_peer_hash_t *bin;
  mx_mag_t *mag;
  unsigned mag_id;
  unsigned idx;
  uint16_t peer_index;
  unsigned long flags;

  flags = 0;

  bin = mx_peer_lookup_eth(dst_high16, dst_low32, 0);
  if (!bin || bin->gw == MX_PEER_INVALID)
    return MX_PEER_INVALID;

  mal_assert(bin->gw < myri_mx_max_nodes);

  /* protect against concurrency with mx_mag_add */
  mx_spin_lock_irqsave(&mx_peer_spinlock, flags);

  mag_id = mx_peer_table[bin->gw].mag_id;

  if (!mag_id) {
    peer_index = bin->gw;
    goto finish;
  }

  mag = &mx_mag_table[mag_id];
  mal_assert(mag->cnt >= 1);

  idx = ((uint16_t)dst_low32 ^ src_low16);
  if (mag->cnt & (mag->cnt - 1))
    idx = idx % mag->cnt; /* normal modulo (mag->cnt not a power of 2) */
  else
    idx = idx & (mag->cnt - 1); /* simplified modulo when mag->cnt is a power of 2 */

  peer_index = mag->peer_index_array[idx];
#if 0
  MX_INFO(("%04x%08x->%04x: mag_id-%d[%d] => %d\n", 
	   dst_high16, dst_low32, src_low16, mag_id, idx, peer_index));
#endif

 finish:

  mx_spin_unlock_irqrestore(&mx_peer_spinlock, flags);

  return peer_index;
}

static int
mx_mag_add(uint32_t mag_id, uint16_t peer_index)
{
  mx_mag_t *mag;
  mx_peer_t *peer;
  uint16_t *index_array = NULL;
  unsigned long flags;
  int insert_pos, i;

  flags = 0;

  mag = mx_mag_table + mag_id;
  peer = mx_peer_table + peer_index;

  if (mag->cnt + 1 >=  mag->malloc_cnt) {
    index_array = mx_kmalloc(sizeof(index_array[0]) * (mag->cnt + 1) * 2, 
				MX_WAITOK | MX_MZERO);
    if (!index_array) {
      MX_WARN(("Cannot grow MAG array to %d entries", (mag->cnt + 1) * 2));
      return ENOMEM;
    }
  }

  mx_spin_lock_irqsave(&mx_peer_spinlock, flags);

  if (index_array) {
    mag->malloc_cnt = (mag->cnt + 1) * 2;
    bcopy(mag->peer_index_array, index_array, mag->cnt * sizeof(index_array[0]));
    if (mag->peer_index_array)
      mx_kfree(mag->peer_index_array);
    mag->peer_index_array = index_array;
  }

  /* compute insertion position (keep array sorted) */
  for (insert_pos=0; insert_pos < mag->cnt; insert_pos++) {
    mx_peer_t *other_peer = mx_peer_table + mag->peer_index_array[insert_pos];
    if (other_peer->mac_high16 > peer->mac_high16 ||
	(other_peer->mac_high16 == peer->mac_high16 &&
	 other_peer->mac_low32 > peer->mac_low32))
      break;
  }

  /* shift entries after insertion point */
  for (i = mag->cnt; i > insert_pos; i--)
    mag->peer_index_array[i] = mag->peer_index_array[i - 1];

  mag->peer_index_array[insert_pos] = peer_index;
  peer->mag_id = mag_id;
  mag->cnt += 1;

  mx_spin_unlock_irqrestore(&mx_peer_spinlock, flags);

  return 0;


}

static void
mx_mag_remove(uint32_t mag_id, uint16_t peer_index)
{
  mx_mag_t *mag;
  mx_peer_t *peer;
  int i, pos;
  unsigned long flags;

  flags = 0;

  mag = mx_mag_table + mag_id;
  peer = mx_peer_table + peer_index;

  mx_spin_lock_irqsave(&mx_peer_spinlock, flags);

  for (pos = 0; pos < mag->cnt; pos++) {
    if (peer_index == mag->peer_index_array[pos])
      break;
  }
  mal_assert(pos < mag->cnt);
  for (i = pos; i < mag->cnt - 1; i++)
    mag->peer_index_array[i] = mag->peer_index_array[i + 1];

  mag->cnt -= 1;
  peer->mag_id = 0;
  
  mx_spin_unlock_irqrestore(&mx_peer_spinlock, flags);
}

int
mx_update_peer_mag_id(uint16_t peer_index, uint32_t new_mag_id)
{
  mx_peer_t *peer;
  uint16_t old_mag_id;

  mal_always_assert(new_mag_id < myri_mx_max_nodes);

  peer = mx_peer_table + peer_index;
  old_mag_id = peer->mag_id;

  if (old_mag_id == new_mag_id)
	  return 0;
  if (old_mag_id)
	  mx_mag_remove(peer->mag_id, peer_index);
  if (!new_mag_id)
	  return 0;
  return mx_mag_add(new_mag_id, peer_index);
}




int
myri_set_route_begin(mx_endpt_state_t *es)
{
  mx_instance_state_t *is = es->is;
  int i;

  /* zero out the pointers to routes */
  for (i = 0; i < is->num_ports; i++) {
    int j;
    bzero(is->routes[i].offsets, 
	  sizeof(is->routes[i].offsets[0]) * myri_mx_max_nodes);
    if (is->id < MX_PEER_FLAG_SEEN_NB_BOARDS) {
      for (j=0; j <= mx_biggest_peer; j++)
	mx_peer_table[j].flags &= ~(MX_PEER_FLAG_SEEN_P0 << (is->id * 2 + i));
    }
  }
  bzero(is->raw.valid_route_count, sizeof(is->raw.valid_route_count));
  return 0;
}


static int
myri_update_port_routes(mx_instance_state_t *is, mx_routes_t *routes, int port)
{
  int peer_index, peer_cnt, status;
  uint32_t dummy;

  if (MAL_IS_ETHER_BOARD(is->board_type))
    return 0;
  
  mal_always_assert(mx_biggest_peer < myri_mx_max_nodes);
  mx_pio_memcpy(routes->mcp_table, routes->host_table, 
	       (mx_biggest_peer + 1) * routes->block_size, MX_PIO_FLUSH | MX_PIO_32BYTE_FLUSH);
  
  peer_index = 0;
  peer_cnt = MX_VPAGE_SIZE / routes->block_size;

  while(peer_index <= mx_biggest_peer) {
    if ((mx_biggest_peer + 1 - peer_index) < peer_cnt)
      peer_cnt = mx_biggest_peer + 1 - peer_index;
    
    status = mx_mcp_command(is, MCP_CMD_UPDATE_ROUTES, port, 
			    peer_index, peer_cnt, &dummy);
    if (status) {
      MX_WARN(("Updating routes failed on board %d\n", is->id));
      return status;
    }

    peer_index += MX_VPAGE_SIZE / routes->block_size;
  }

  /* temporary code until 2Z supports probing:
     if there is any 2z/xm, converts to adaptive routing
     but don't override any manual dispersive setting */
  if (mx_has_xm && !is->manual_dispersion) {
    uint32_t dispersion = (1 << 30) + (0xc0U /* 75% */ << 8) + 8;
    is->manual_dispersion = 1;
    status = mx_mcp_command(is, MCP_CMD_SET_DISPERSION, 
			    0, dispersion, 0, &dummy);
    MX_INFO(("%s: setting dispersion to adaptive\n", is->is_name));
  }

  return 0;
}

/* used either after the FMA gave the routes or after parity recovery */
int
myri_update_board_routes(mx_instance_state_t *is)
{
  int i, status;

  for (i = 0; i < is->num_ports; i++)
    if (is->raw.valid_route_count[i]) {
      status = myri_update_port_routes(is, &is->routes[i], i);
      if (status) {
	return status;
      }
    }

  return 0;
}

int
myri_set_route_end(mx_endpt_state_t *es)
{
  mx_instance_state_t *is = es->is;
  int status;
  int peer_index, gw_index;

  /* update routes for MXvM peers */
  for (peer_index = 0; peer_index <= mx_biggest_peer; peer_index ++) {
    mx_peer_t *peer = mx_peer_table + peer_index;
    if (peer->type == MX_HOST_MXvM) {
      mx_mag_t * mag;
      mx_peer_hash_t * hash;
      uint16_t my_low16;
      unsigned long flags;
      int i;

      flags = 0;

      hash = mx_peer_lookup(peer->gw_id >> 32, (uint32_t)peer->gw_id);

      if (!hash)
	continue;

      gw_index = hash->index;

      mx_spin_lock_irqsave(&mx_peer_spinlock, flags);
      if (mx_peer_table[gw_index].mag_id) {
	unsigned idx;
	
	mag = mx_mag_table + mx_peer_table[gw_index].mag_id;
	
	my_low16 = (is->mac_addr[4] << 8) + is->mac_addr[5];
	idx = ((uint16_t)peer->mac_low32 ^ my_low16) % mag->cnt;
	gw_index = mag->peer_index_array[idx];
      }
      mx_spin_unlock_irqrestore(&mx_peer_spinlock, flags);
      
      for (i=0; i < is->num_ports; i++) {
	mx_routes_t *routes = es->is->routes + i;
	bcopy(routes->host_table + gw_index * routes->block_size, 
	      routes->host_table + peer_index * routes->block_size,
	      routes->block_size);
      }
    }
  }

  status = myri_update_board_routes(is);
  if (status)
    return status;

  mx_query_peer(is, 0);
  return 0;
}


/*
 * Sets a route for use by the mcp.
 * 
 * 1) All routes begin aligned on an 8 byte boundary.
 * 2) The length of the route is stored just before that boundary.
 * 3) The routes are repeated to fill up all the slots.
 *
 */

int
myri_set_route(mx_endpt_state_t *es, mx_set_route_t *r, int clear)
{
  mx_instance_state_t *is = es->is;
  uint32_t mac_low32;
  uint16_t mac_high16;
  int hash_index, peer_index, status;
  signed char *block;
  mx_routes_t *routes;
  int offset, block_size;
  unsigned int source_port;
  unsigned int length;
  uint8_t hops[64];
  mx_peer_hash_t *bin;

  source_port = r->source_port;
  if (source_port >= is->num_ports)
    return EINVAL;

  routes = &is->routes[source_port]; /* only handle port 0 for now, mapper will
			      tell us which port in future */

  MAL_DEBUG_PRINT(MAL_DEBUG_RAW, ("%s called\n", __FUNCTION__));

  length = r->route_length;

  block_size = routes->block_size;

  if (length > MYRI_RAW_MAXROUTE)
    return ENOSPC;

  status = mx_copyin((uaddr_t)r->route_pointer, hops, length, es->is_kernel);
  if (status)
    return status;

  mac_low32 = r->mac_low32;
  mac_high16 = r->mac_high16;

  /* lookup mx mac address in hash or overflow table */
  bin = mx_peer_lookup(mac_high16, mac_low32);
  if (!bin) {
    MAL_DEBUG_PRINT(MAL_DEBUG_RAW, ("Adding address 0x%x%x to peer hash at index %d\n", 
				  mac_high16, mac_low32, hash_index));
    status = mx_add_peer(&hash_index, mac_high16, mac_low32);
    if (status == ENOSPC) {
      MX_WARN(("%s: Peer table full, dropping node 0x%x%x\n",
	       is->is_name, mac_high16, mac_low32));
      return ENOENT;
    }
    bin = mx_peer_lookup(mac_high16, mac_low32);
  }
  peer_index = bin->index;

  if (is->id < MX_PEER_FLAG_SEEN_NB_BOARDS) {
    mx_peer_table[peer_index].flags |= 
      MX_PEER_FLAG_SEEN_P0 << (source_port + is->id * 2);
  }

  mx_update_peer_type(r->host_type, peer_index, 0);

  if (r->host_type == MX_HOST_MXvM && length == 6) {
    uint64_t gw_id = 0;
    int j;
    for (j=0;j<6;j++)
      gw_id += (uint64_t)hops[5- j] << (j * 8);
    mx_peer_table[peer_index].gw_id = gw_id;
    length = 0;
  }
  offset = routes->offsets[peer_index];
  
  /* find base of routing block */
  block = (signed char *) &routes->host_table[peer_index * block_size];

  if ((status = mx_update_peer_mag_id(peer_index, r->mag_id)))
    return status;

  if (clear) {
    bzero(block, block_size);
    routes->offsets[peer_index] = 0;
    is->raw.valid_route_count[source_port]--;

    return 0;
  }
  
  
  /* copy the route.  
     When writing routes, we start them on an 8 byte boundary,
     so they will be properly aligned for the firmware.  We
     put the length at the end */

  if (offset + MYRI_RAW_MAXROUTE + 1 > block_size
      || length > MYRI_RAW_MAXROUTE) {
    /* no space available to store the route */
   MAL_DEBUG_PRINT(MAL_DEBUG_RAW,
		   ("%s: no space for rt to host %x%x (%d, %d, %d))\n", 
		    is->is_name, mac_high16, mac_low32, offset, length, block_size));
    return ENOSPC;
  }


  /* copy the route */
  bcopy(hops, &block[offset], length);

  /* mark directly connected nodes so that mx_info can
     properly display them */

  if (length == 0 && offset == 0)
    block[0] = 0xff;

  offset += MYRI_RAW_MAXROUTE + 1;
  
  /* update the length of this route.  we store the length
     at the end of the route just prior to the next one */

  block[offset - 1] = length;

  if (offset < block_size) {
    /* fill the remaining slots by repeating the sequence given so
       far */
    int o;
    for (o =0 ; o < block_size - offset; o += MYRI_RAW_MAXROUTE + 1) {
      mal_always_assert(offset + o + MYRI_RAW_MAXROUTE + 1 <= block_size);
      memcpy(block + offset + o, block + o, MYRI_RAW_MAXROUTE + 1);
    }
  }
  routes->offsets[peer_index] = offset;
  is->raw.valid_route_count[source_port]++;
  return 0;
}
