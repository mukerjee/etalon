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


#include "mal_auto_config.h"
#include "mal.h"
#include "mal_valgrind.h"

#ifndef MAL_KERNEL
#define MAL_KUTILS 1
#elif MX_KERNEL_LIB
#include "mx_klib.h"
#define MAL_KUTILS 1
#endif

#ifdef MAL_KUTILS
struct mal_mpool
{
  char  *base;
  size_t size;
  char  *elem_next;
  size_t elem_sz;
  int    elem_avail;
};

int
mal_mpool_init(char *base, size_t size, size_t elem_sz, struct mal_mpool **pool_o)
{
  struct mal_mpool *pool;
  int elem_cnt, i;
  char *p = base;

  pool = mal_malloc(sizeof(*pool));
  if (pool == NULL)
    return ENOMEM;
  pool->base = base;
  pool->size = size;
  pool->elem_next = p;
  pool->elem_sz = elem_sz;
  pool->elem_avail = elem_cnt = (int)(size / elem_sz);

  MAL_VALGRIND_MEMORY_MAKE_NOACCESS(base, size);
  for (i = 0; i < elem_cnt-1; ++i) {
    MAL_VALGRIND_MEMORY_MAKE_WRITABLE(p + i * elem_sz, sizeof(char*));
    *(char**)(p + i * elem_sz) = p + (i+1) * elem_sz;
  }
  MAL_VALGRIND_MEMORY_MAKE_WRITABLE(p + (elem_cnt-1) * elem_sz, sizeof(char*));
  *(char**)(p + (elem_cnt-1) * elem_sz) = NULL;
  *pool_o = pool;
  return 0;
}

void
mal_mpool_fini(struct mal_mpool *pool)
{
  if (pool != NULL)
    mal_free(pool);
}

void *
mal_mpool_alloc(struct mal_mpool *pool)
{
  char *ptr = NULL;

  if (pool->elem_avail > 0) {
    ptr = pool->elem_next;
    MAL_VALGRIND_MEMORY_MAKE_WRITABLE(ptr, pool->elem_sz);
    MAL_VALGRIND_MEMORY_MAKE_READABLE(ptr, pool->elem_sz);
    pool->elem_next = *(char **)(pool->elem_next);
    pool->elem_avail--;
  }
  return ptr;
}

int
mal_mpool_isempty(struct mal_mpool *pool)
{
  return !!(pool->elem_avail == 0);
}

void
mal_mpool_free(struct mal_mpool *mpool, void *ptr)
{
  MAL_VALGRIND_MEMORY_MAKE_NOACCESS(ptr, mpool->elem_sz);
  MAL_VALGRIND_MEMORY_MAKE_WRITABLE(ptr, sizeof(char*));
  *(char**)ptr = mpool->elem_next;
  mpool->elem_next = ptr;
  mpool->elem_avail++;
}

void
mal_macaddr_to_nic_id(const uint8_t macaddr[6], uint64_t *nic_idp)
{
  int i;
  uint64_t nic_id = 0;
  for (i = 0; i < 6; i++) {
    nic_id <<= 8;
    nic_id += macaddr[i];
  }
  *nic_idp = nic_id;
}

void
mal_nic_id_to_macaddr(uint64_t nic_id, uint8_t macaddr[6])
{
  int i;
  for (i = 5; i >= 0; i--) {
    macaddr[i] = (uint8_t)(nic_id & 0xff);
    nic_id >>= 8;
  }
}

MAL_FUNC(void)
mal_nic_id_to_str(char *str, uint64_t nic_id, uint32_t strlen)
{
  mal_snprintf(str, strlen, "%02x:%02x:%02x:%02x:%02x:%02x",
	   (uint8_t) (nic_id >> 40), (uint8_t) (nic_id >> 32),
	   (uint8_t) (nic_id >> 24), (uint8_t) (nic_id >> 16),
	   (uint8_t) (nic_id >> 8), (uint8_t) (nic_id >> 0));
  str[strlen - 1] = 0;
}

int
mal_get_nicinfo(struct mal_nic_info **ninfo_o, int *num_o)
{
  mal_handle_t fd = MAL_INVALID_HANDLE;
  struct mal_nic_info *ninfo = NULL;
  uint32_t num;
  int i, rc = 0;
  uint32_t nic_type;
  uint32_t max_endpts;
  myri_get_nic_id_t nic_id;

  if ((rc = mal_open_any_board(&fd)))
    return rc;

  if ((rc = mal_ioctl(fd, MYRI_GET_BOARD_COUNT, &num, sizeof(num))))
    goto fail;
  if (num == 0)
    goto fail;

  if (!(ninfo = mal_malloc(sizeof(*ninfo) * num))) {
    rc = ENOMEM;
    goto fail;
  }

  for (i = 0; i < num; i++) {
    nic_id.board = i;
    nic_type = i;
    if ((rc = mal_ioctl(fd, MYRI_GET_NIC_ID, &nic_id, sizeof(nic_id))))
      goto fail;
    if ((rc = mal_ioctl(fd, MYRI_GET_BOARD_TYPE, &nic_type, sizeof(nic_type))))
      goto fail;
    if ((rc = mal_ioctl(fd, MYRI_GET_ENDPT_MAX, &max_endpts, sizeof(max_endpts))))
      goto fail;
    ninfo[i].boardnum = i;
    ninfo[i].nic_id = nic_id.nic_id;
    ninfo[i].nic_type = nic_type;
    ninfo[i].nic_max_endpoints = max_endpts;
  }

fail:
  if (fd != MAL_INVALID_HANDLE)
    mal_close(fd);
  if (!rc) {
    *ninfo_o = ninfo;
    *num_o = (int) num;
  }
  else if (ninfo != NULL)
    mal_free(ninfo);

  return rc;
}

void
mal_free_nicinfo(struct mal_nic_info *ninfo)
{
  mal_free(ninfo);
}
#endif
