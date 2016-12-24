/*************************************************************************
 * The contents of this file are subject to the MYRICOM SNIFFER10G
 * LICENSE (the "License"); User may not use this file except in
 * compliance with the License.  The full text of the License can found
 * in LICENSE.TXT
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and
 * limitations under the License.
 *
 * Copyright 2008 - 2010 by Myricom, Inc.  All rights reserved.
 ***********************************************************************/

#include "myri_version.h"
#include "snf_libtypes.h"
#include "snf.h"

static int  snf__ring_open(struct snf_handle *handle, int ring_id, 
                           struct snf_ring **ring_o);

struct snf__params snf__p; /* default parameters */
uint16_t snf__user_api = 0;
int snf__init = 0;

MAL_FUNC(uint16_t)
snf_version_api(void)
{
  return SNF_VERSION_API;
}

MAL_FUNC(int)
snf_init(uint16_t user_api)
{
#ifndef MAL_KERNEL
  mal_handle_t fd = MAL_INVALID_HANDLE;
  myri_get_version_t x;
  int rc;
#endif /* MAL_KERNEL */

  /* Three different versioning checks.
   * 1. That the MYRI_LIB_ defines are maintained internally to match
   *    SNF_VERSION_API.
   * 2. That the user-provided API version is source-compatible with the
   *    library version.
   * 3. That the library is compatible with the driver that's loaded.
   */

  SNF__CTA((MYRI_LIB_MINOR + (MYRI_LIB_MAJOR << 16)) == SNF_VERSION_API);

  if (snf__init) {
    /* Already initialized, we don't need to go through this again */
    if (user_api != snf__user_api) /* initilized twice, diff ver ? */
      return EINVAL;
    else
      return 0;
  }
  snf__init = 1;
  snf__user_api = user_api;
  snf__p.debug_mask = 0x1; /* FIXME, get from env or module param */

  if ((user_api & 0xff00) != (SNF_VERSION_API & 0xff00)) {
    SNF_DPRINTF(SNF_G, ERROR, 
        "user API %#06x is incompatible with library API %#06x\n",
        user_api, SNF_VERSION_API);
    return EINVAL;
  }

#ifndef MAL_KERNEL
  if ((rc = mal_open_any_board(&fd)))
    return rc;

  rc = mal_ioctl(fd, MYRI_GET_VERSION, &x, sizeof(x));
  mal_close(fd);

  if (rc)
    return rc;
  
  if (x.driver_api_magic / 256 != MYRI_DRIVER_API_MAGIC / 256) {
    fprintf(stderr, 
        "snf library and driver API mismatch (lib=%d.%d, kernel=%d.%d\n"
        "\t snf lib version=%s\n"
        "\t snf lib build=%s\n"
        "\t snf kernel version=%s\n"
        "\t snf kernel build=%s\n",
	MYRI_DRIVER_API_MAGIC / 256, MYRI_DRIVER_API_MAGIC % 256,
	x.driver_api_magic / 256, x.driver_api_magic % 256,
	MYRI_VERSION_STR,
	MYRI_BUILD_STR,
	x.version_str,
	x.build_str);
    return ENXIO;
  }
#endif /* MAL_KERNEL */

  return 0;
}

#ifdef MAL_KERNEL
MAL_FUNC(int)
snf_getifaddrs(struct snf_ifaddrs **ifaddrs_o)
{
  struct mal_nic_info *nic_info = NULL;
  struct snf_ifaddrs *snf_ifs = NULL;
  int i, num_if_myri = 0;
  int rc;
  char *p;
  const size_t plen = 64;

  if ((rc = mal_get_nicinfo(&nic_info, &num_if_myri)) || !num_if_myri)
    goto bail;

  printk("passed mal_get_nicinfo: num_if_myri=%d\n", num_if_myri);

  snf_ifs = mal_malloc(num_if_myri * (sizeof(struct snf_ifaddrs) + plen));
  if (snf_ifs == NULL) {
    rc = ENOMEM;
    goto bail;
  }
  p = (char *)(snf_ifs + num_if_myri);

  for (i = 0; i < num_if_myri; i++) {
    snf_ifs[i].snf_ifa_next = snf_ifs + i + 1;
    snf_ifs[i].snf_ifa_name = p;
    mal_snprintf(p, plen - 1, "myri%d", i);
    *(p + plen - 1) = '\0';
    p += plen;
    snf_ifs[i].snf_ifa_boardnum = nic_info[i].boardnum;
    snf_ifs[i].snf_ifa_maxrings = nic_info[i].nic_max_endpoints;
    mal_nic_id_to_macaddr(nic_info[i].nic_id, snf_ifs[i].snf_ifa_macaddr);
    snf_ifs[i].snf_ifa_maxinject = nic_info[i].nic_max_endpoints;
  }
  snf_ifs[i-1].snf_ifa_next = NULL;

bail:
  if (!rc)
    *ifaddrs_o = snf_ifs;
  if (nic_info != NULL)
    mal_free_nicinfo(nic_info);

  printk("getifaddrs returns %d with %d nics\n", rc, num_if_myri);

  return rc;
}

MAL_FUNC(void)
snf_freeifaddrs(struct snf_ifaddrs *ifaddrs)
{
  mal_free(ifaddrs);
}

#else
MAL_FUNC(int)
snf_getifaddrs(struct snf_ifaddrs **ifaddrs_o)
{
  struct mal_ifaddrs *mal_ifs = NULL, *mif, *mif2;
  struct snf_ifaddrs *snf_ifs = NULL;
  int num_if_mal = 0, num_if_snf = 0;
  int rc = 0, found_dupe = 0;

  /* Must have called snf_init */
  if (!snf__init) {
    SNF_DPRINTF(SNF_G, ERROR, "snf: snf_init not called before %s!\n", "snf_getifaddrs");
    return EINVAL;
  }

  if ((rc = mal_getifaddrs(&mal_ifs, 0, &num_if_mal)) || !num_if_mal)
    goto bail;
  else
    mif = mal_ifs;

  snf_ifs = mal_malloc(sizeof(struct mal_ifaddrs) * num_if_mal);
  if (snf_ifs == NULL) {
    rc = ENOMEM;
    goto bail;
  }
  while (mif) {
    mif2 = mal_ifs;
    found_dupe = 0;
    /* The same interface can appear in multiple address families, such as
     * AF_INET, AF_INET6 and AF_PACKET on Linux.  We only want to expose
     * one of them for snf users. */
    while (mif2 && mif2 != mif) {
      if (!strncmp(mif->mal_ifa_name, mif2->mal_ifa_name,
                   strlen(mif->mal_ifa_name)+1)) {
        found_dupe = 1;
        break;
      }
      mif2 = mif2->mal_ifa_next;
    }
    if (!found_dupe) {
      snf_ifs[num_if_snf].snf_ifa_name = mal_strdup(mif->mal_ifa_name);
      snf_ifs[num_if_snf].snf_ifa_boardnum = mif->mal_ifa_boardnum;
      snf_ifs[num_if_snf].snf_ifa_maxrings = mif->mal_ifa_max_endpoints;
      memcpy(snf_ifs[num_if_snf].snf_ifa_macaddr, mif->mal_ifa_macaddr, 6);
      /* We always have as many rings as inject handles */
      snf_ifs[num_if_snf].snf_ifa_maxinject = mif->mal_ifa_max_endpoints;
      snf_ifs[num_if_snf].snf_ifa_next = NULL;
      if (num_if_snf)
        snf_ifs[num_if_snf-1].snf_ifa_next = &snf_ifs[num_if_snf];
      num_if_snf++;
    }
    mif = mif->mal_ifa_next;
  }

bail:
  if (mal_ifs)
    mal_freeifaddrs(mal_ifs);
  if (!rc)
    *ifaddrs_o = snf_ifs;
  else if (snf_ifs)
    mal_free(snf_ifs);
  return rc;
}

MAL_FUNC(void)
snf_freeifaddrs(struct snf_ifaddrs *ifaddrs)
{
  struct snf_ifaddrs *ifa = ifaddrs;
  while (ifa) {
    mal_free((void *) ifa->snf_ifa_name);
    ifa = ifa->snf_ifa_next;
  }
  mal_free(ifaddrs);
}
#endif /* MAL_KERNEL */

MAL_FUNC(int)
snf_open_defaults(uint32_t boardnum, snf_handle_t *devhandle)
{
  return snf_open(boardnum, 0, NULL, 0, -1, devhandle);
}

MAL_FUNC(int)
snf_open(uint32_t boardnum,
         int num_rings,
         const struct snf_rss_params *rss_params,
         int64_t data_ring_size, 
         int flags,
	 snf_handle_t *handle_o)
{
  int rc = 0;
  struct snf__params p;
  struct snf_handle *handle = NULL;
  struct snf_param_keyval op[_SNF_PARAM_LAST];
  int num_keys = 1;

  /* Must have called snf_init */
  if (!snf__init) {
    SNF_DPRINTF(SNF_G, ERROR, "snf: snf_init not called before %s!\n", "snf_open");
    return EINVAL;
  }

  op[0].key = SNF_PARAM_BOARDNUM;
  op[0].val.boardnum = boardnum;
  if (num_rings > 0) { /* default if 0 or less */
    op[num_keys].key = SNF_PARAM_NUM_RINGS;
    op[num_keys].val.num_rings = num_rings;
    num_keys++;
  }
  if (flags >= 0) { /* default if less than 0 */
    op[num_keys].key = SNF_PARAM_FLAGS;
    op[num_keys].val.open_flags = (uint32_t) flags;
    num_keys++;
  }
  if (data_ring_size > 0) { /* default if 0 or less */
    op[num_keys].key = SNF_PARAM_DATARING_SIZE;
    op[num_keys].val.data_ring_size = (uint64_t) data_ring_size;
    num_keys++;
  }
  if (rss_params) {
    if (rss_params->mode == SNF_RSS_FLAGS) {
      op[num_keys].key = SNF_PARAM_RSS_FLAGS;
      op[num_keys].val.rss_flags = rss_params->params.rss_flags;
      num_keys++;
    }
    else if (rss_params->mode == SNF_RSS_FUNCTION) {
      op[num_keys].key = SNF_PARAM_RSS_FLAGS;
      op[num_keys].val.rss_flags = SNF_RSS_FLAGS_CUSTOM_FUNC;
      num_keys++;
      op[num_keys].key = SNF_PARAM_RSS_FUNC_PTR;
      op[num_keys].val.rss_hash_fn = rss_params->params.rss_function.rss_hash_fn;
      num_keys++;
      op[num_keys].key = SNF_PARAM_RSS_FUNC_CONTEXT;
      op[num_keys].val.rss_context = rss_params->params.rss_function.rss_context;
      num_keys++;
    }
    else {
      SNF_DPRINTF(SNF_G, ERROR, "snf: unrecognized RSS param %d\n", rss_params->mode);
      return EINVAL;
    }
  }

  if (!(handle = mal_calloc(1, sizeof(*handle)))) {
    rc = ENOMEM;
    goto bail;
  }
  handle->mhandle = MAL_INVALID_HANDLE;

  MAL_MUTEX_INIT(&handle->lock);
  TAILQ_INIT(&handle->ringq);

  if ((rc = snf__api_params(&p, &handle->drv_p, op, num_keys, NULL)))
    goto bail;


  handle->p = p;

  if ((rc = mal_open(p.boardnum, 0, &handle->mhandle))) {
    SNF_DPRINTF(&p, IOCTL, "Can't open SNF RX handle on board %u (err=%d)\n",
        p.boardnum, rc);
    goto bail;
  }

#ifdef MAL_KERNEL
  rc = myri_klib_set_endpoint(&handle->mhandle, p.boardnum, 0, &handle->drv_p,
                              MYRI_SNF_SET_ENDPOINT_RX);
#else
  rc = mal_ioctl(handle->mhandle, MYRI_SNF_SET_ENDPOINT_RX,
                      &handle->drv_p, sizeof(handle->drv_p));
#endif
  if (rc) {
    SNF_DPRINTF(&p, IOCTL, "Can't set ENDPOINT_RX board %u (err=%d)\n",
        p.boardnum, rc);
    mal_close(handle->mhandle);
    goto bail;
  }

bail:
  if (rc) {
    if (handle)
      mal_free(handle);
  }
  else
    *handle_o = handle;
  return rc;
}

MAL_FUNC(int)
snf_close(snf_handle_t handle)
{
  int rc = 0;

  if (!handle)
    return EINVAL;

  MAL_MUTEX_LOCK(&handle->lock);
  if (!TAILQ_EMPTY(&handle->ringq))
    rc = EBUSY;
  else
    mal_close(handle->mhandle);
  MAL_MUTEX_UNLOCK(&handle->lock);

  if (rc == 0)
    mal_free(handle);

  return rc;
}

MAL_FUNC(int)
snf_start(snf_handle_t handle)
{
  int rc;
  MAL_MUTEX_LOCK(&handle->lock);
  rc = mal_ioctl(handle->mhandle, MYRI_SNF_RX_START, NULL, 0);
  MAL_MUTEX_UNLOCK(&handle->lock);
  return rc;
}

MAL_FUNC(int)
snf_stop(snf_handle_t handle)
{
  int rc;
  MAL_MUTEX_LOCK(&handle->lock);
  rc = mal_ioctl(handle->mhandle, MYRI_SNF_RX_STOP, NULL, 0);
  MAL_MUTEX_UNLOCK(&handle->lock);
  return rc;
}

MAL_FUNC(int)
snf_ring_getstats(snf_ring_t ring, struct snf_ring_stats *stats)
{
  myri_snf_stats_t mstats;
  int rc;

  memset(&mstats, 0, sizeof(mstats));

  if ((rc = mal_ioctl(ring->mhandle, MYRI_SNF_STATS, &mstats, sizeof(mstats))))
    return rc;

  stats->nic_pkt_recv = mstats.snf_pkt_recv;
  stats->nic_pkt_overflow = mstats.nic_pkt_overflow;
  stats->nic_pkt_bad = mstats.nic_pkt_bad;

  if (ring->drv_p.num_rings == 1) {
    stats->ring_pkt_recv = ring->rx.rx_s->recvq.rq_pkt_cnt;
    stats->ring_pkt_overflow = mstats.snf_pkt_overflow;
  }
  else {
    stats->ring_pkt_recv = ring->rx.rx_s->vrings[ring->p.ring_id].vr_p.pkt_recvd;
    stats->ring_pkt_overflow = ring->rx.rx_s->vrings[ring->p.ring_id].vr_p.pkt_drops;
  }

  if (snf__user_api >= 0x0002)
    stats->nic_bytes_recv = mstats.nic_bytes_recv;

  if (snf__user_api >= 0x0003)
    stats->snf_pkt_overflow = mstats.snf_pkt_overflow;
  else {
    /* Before 0x3, we would account for snf_pkt_overflows by adding them to
     * each ring's overflow counter, but only in multi-ring mode when the
     * PQ_DESC is mapped by overyone */
    if ((ring->rx.rx_map_flags & MYRI_SNF_RXMAP_PQ_DESC) &&
           (ring->drv_p.num_rings > 1))
      stats->ring_pkt_overflow += mstats.snf_pkt_overflow;
  }
  return 0;
}

MAL_FUNC(int)
snf_ring_open_id(snf_handle_t handle, int ring_id, snf_ring_t *ring_o)
{
  int rc = 0;

  MAL_MUTEX_LOCK(&handle->lock);
  rc = snf__ring_open(handle, ring_id, ring_o);
  MAL_MUTEX_UNLOCK(&handle->lock);

  return rc;
}

MAL_FUNC(int)
snf_ring_open(snf_handle_t handle, snf_ring_t *ring)
{
  return snf_ring_open_id(handle, -1, ring);
}

MAL_FUNC(int)
snf_ring_close(snf_ring_t ring)
{
  snf_handle_t handle;
  if (ring == NULL)
    return EINVAL;
  handle = ring->handle;
  MAL_MUTEX_LOCK(&handle->lock);
  snf__rx_fini(&ring->rx);
  TAILQ_REMOVE(&handle->ringq, ring, link);
  mal_close(ring->mhandle);
  MAL_MUTEX_UNLOCK(&handle->lock);
  mal_free(ring);
  return 0;
}

static
int
snf__ring_open(struct snf_handle *handle, int ring_id, struct snf_ring **ring_o)
{
  struct snf__params *p = &handle->p;
  int rc;
  myri_snf_rx_attach_t ap;
  struct snf_ring *ring = NULL;

  /* Assume handle->lock is held */
  ring = mal_calloc(1, sizeof(*ring));
  if (ring == NULL) {
    rc = ENOMEM;
    goto bail;
  }

  ring->mhandle = MAL_INVALID_HANDLE;
  ring->drv_p = handle->drv_p;
  ring->handle = handle;
  ring->p = handle->p;

  ap.ring_id = ring_id; /* Can be -1 for don't care */
  ap.params = handle->drv_p;

  if ((rc = mal_open(p->boardnum, 0, &ring->mhandle))) {
    SNF_DPRINTF(p, IOCTL, 
        "Can't open SNF RX ring handle on board %u (err=%d)\n", p->boardnum, rc);
    goto bail;
  }

#ifdef MAL_KERNEL
  rc = myri_klib_set_endpoint(&ring->mhandle, p->boardnum, 0, &ap,
                              MYRI_SNF_SET_ENDPOINT_RX_RING);
#else
  rc = mal_ioctl(ring->mhandle, MYRI_SNF_SET_ENDPOINT_RX_RING, &ap, sizeof(ap));
#endif

  if (rc) {
    SNF_DPRINTF(p, IOCTL, "Can't attach SNF ring to board %u (err=%d)\n",
       p->boardnum, rc);
    goto bail;
  }
  ring->p.ring_id = ap.ring_id;

  if ((rc = snf__rx_init(&ring->rx, ring->mhandle, &ring->p, &ap)))
    goto bail;

bail:
  if (rc) {
    if (ring->mhandle != MAL_INVALID_HANDLE)
      mal_close(ring->mhandle);
    mal_free(ring);
  }
  else {
    TAILQ_INSERT_TAIL(&handle->ringq, ring, link);
    *ring_o = ring;
  }

  return rc;
}

MAL_FUNC(int)
snf_ring_recv(snf_ring_t ring, int timeout_ms, struct snf_recv_req *recv_req)
{
  return ring->rx.ring_recv(&ring->rx, timeout_ms, recv_req);
}

#ifndef MAL_KERNEL
static
void
snf__print_map(const struct snf__params *p, const char *map_name, void *ptr,
               uintptr_t size_i)
{
  unsigned long long size = (unsigned long long) size_i;
  unsigned long long psize;
  const char *suffix = "MiB";

  if (size == 0) {
    suffix = "";
    ptr = NULL;
    psize = 0;
  }
  else if (size < 1024*1024) {
    psize = size / 1024;
    suffix = "KiB";

  }
  else
    psize = size / (1024*1024);

  SNF_DPRINTF(p, PARAM, "%14s [%#16lx..%#16lx) size %5llu %3s %12llu (%#llx)\n",
	      map_name, (unsigned long)(uintptr_t) ptr, (unsigned long)((uintptr_t) ptr + size_i), 
	      psize, suffix, size, size);
}

int
snf__mmap(const struct snf__params *p, mal_handle_t mhandle,
          const char *map_name, uintptr_t *map_ptr, uintptr_t size, 
          uintptr_t offset, int read_only)
{
  void *ptr;
  int rc;

  if (size == 0 || offset == MYRI_OFFSET_UNMAPPED) {
    ptr = NULL;
    size = 0;
    goto print_map;
  }
  ptr = mal_mmap(0, (size_t) size, mhandle, offset, read_only);
  if (ptr == MAL_MAP_FAILED) {
    rc = errno;
    SNF_DPRINTF(p, ERROR, "mmap %10s %10s (offset=%llu,size=%llu) "
                "(err=%d:%s)\n", map_name, read_only ? "read-only" :
                "read-write", (unsigned long long) offset, 
                (unsigned long long) size, 
                rc, strerror(rc));
    return rc;
  }
  if (MAL_OS_FREEBSD || MAL_OS_SOLARIS) {
    /* prefault mmap'ed areas to prevent performance
       artifacts when warming up */
    static volatile long sum = 0;
    long long i;
    uint8_t *q = (uint8_t *)ptr;
    for (i = 0; i < size/4096; i++, q += 4096)
      sum += *q;
  }
print_map:
  snf__print_map(p, map_name, ptr, size);

  *map_ptr = (uintptr_t) ptr;
  return 0;
}

void 
snf__assertion_failed (const char *assertion, int line, const char *file)
{
  printf("snf: assertion: <<%s>>  failed at line %d, file %s\n",
         assertion, line, file);
  abort();
}
#endif /* MAL_KERNEL */

MAL_FUNC(int)
snf_netdev_reflect_enable(snf_handle_t hsnf, snf_netdev_reflect_t *handle)
{
  *handle = (snf_handle_t)hsnf;
  return 0;
}

MAL_FUNC(int)
snf_netdev_reflect(snf_netdev_reflect_t handle, const void *buf, uint32_t len)
{
  myri_soft_rx_t x;
  int rc;
  snf_handle_t snfh = (snf_handle_t)handle;

  x.pkt_len = len;
  x.hdr_len = 0;
  x.flags = 0;
  x.seg_cnt = 1;
  x.copy_desc[0].ptr = (uint64_t)(uintptr_t)buf;
  x.copy_desc[0].len = len;
  rc = mal_ioctl(snfh->mhandle, MYRI_SNF_SOFT_RX, &x, sizeof (x));
  if (rc == EINVAL && len == 0xdeadbeef)
    return 0;
  return rc;
}

