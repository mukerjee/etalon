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
 * Copyright 2010 by Myricom, Inc.  All rights reserved.
 ***********************************************************************/

#define _GNU_SOURCE

#include "mal.h"
#include "mal_thread.h"
#include "mal_timing.h"

#include "snf_inline.h"

#ifndef MAL_KERNEL
#include <signal.h>
#include <arpa/inet.h>
#ifdef __FreeBSD__
#include <sys/param.h>
#if __FreeBSD_version < 702100
#define PTHREAD_SETAFFINITY_NP_MISSING
#else
#include <sched.h>
#include <pthread_np.h>
#define cpu_set_t cpuset_t
#endif /* __FreeBSD_version */
#elif defined(__GLIBC__) && !__GLIBC_PREREQ(2,4)
#define PTHREAD_SETAFFINITY_NP_MISSING
#endif
#else
#define PTHREAD_SETAFFINITY_NP_MISSING /* No affinity in kernel */
#endif

#define DBG_NETDEV_REFLECT_ALL  0  /* set to non-0 to reflect all packets to netdev */

#define SNF_Safe(x) do {                                  \
  rc = (x);                                               \
  if (rc != 0) {                                          \
    MAL_PRINT(("SNF Failure at line %d: (errno=%d)\n",  \
        __LINE__, rc));                                  \
    goto bail;                                            \
  } } while (0)

#include "snf.h"

#define MAX_INLINES  2
#define MAX_WORKERS 32

struct inl_stats {
  uint64_t  rx_app_cnt;
  uint64_t  tx_app_cnt;
  uint64_t  rfl_app_cnt;
  uint64_t  tx_app_eagain;
  uint64_t  tx_app_dropped;
};

struct inl_worker_info {
  struct inl_info *ti;
  int          rc;
  int          worker_id;
  int          cpu_id;
  snf_ring_t   ringh;
  snf_inject_t injh;
  MAL_THREAD_T wtask;
  struct inl_stats wstats;
  struct snf_ring_stats rstats;
  struct snf_inject_stats istats;
};

struct inl_options;

struct {
  int num_inls;
  mal_cycles_t expire_cyc;

  struct inl_options opts;
  int netdev_reflect;

  struct inl_info {
    int inl_id;
    int inl_is_stopped;
    int inl_is_started;
    int board_rx;
    int board_tx;
    int nrings; /* also nthreads */
    unsigned cpumask;
    snf_handle_t  snfh;
    snf_netdev_reflect_t reflect_handle;
    struct inl_stats stats;
    struct inl_worker_info winfo[MAX_WORKERS];
    MAL_MUTEX_T mtx;
  } inl[MAX_INLINES];
} G;

#define foreach_all_workers(_inl_id,_w_id,_wi)                   \
          for ((_inl_id) = 0; (_inl_id) < G.num_inls; (_inl_id)++)    \
            for ((_w_id) = 0, _wi = &G.inl[(_inl_id)].winfo[(_w_id)]; \
                 (_w_id) < G.inl[(_inl_id)].nrings; (_w_id)++)

#define foreach_inl_worker(_inl_id,_w_id,_wi)                    \
          for ((_w_id) = 0, _wi = &G.inl[(_inl_id)].winfo[(_w_id)];   \
               (_w_id) < G.inl[(_inl_id)].nrings; (_w_id)++)

/* Each worker updates its stats every so often */
void
inl_update_stats(struct inl_worker_info *w, struct inl_stats *tstats)
{
  struct inl_info *ti = w->ti;
  struct inl_stats *wstats = &ti->winfo[w->worker_id].wstats;

  /* First update local stats */
  wstats->rx_app_cnt += tstats->rx_app_cnt;
  wstats->tx_app_cnt += tstats->tx_app_cnt;
  wstats->rfl_app_cnt += tstats->rfl_app_cnt;
  wstats->tx_app_eagain += tstats->tx_app_eagain;
  wstats->tx_app_dropped += tstats->tx_app_dropped;
    
  MAL_MUTEX_LOCK(&ti->mtx);
  ti->stats.rx_app_cnt += tstats->rx_app_cnt;
  ti->stats.tx_app_cnt += tstats->tx_app_cnt;
  ti->stats.rfl_app_cnt += tstats->rfl_app_cnt;
  ti->stats.tx_app_eagain += tstats->tx_app_eagain;
  ti->stats.tx_app_dropped += tstats->tx_app_dropped;

  if (ti->inl_is_stopped == 0) {
    if (G.opts.num_pkts_expected && ti->stats.rx_app_cnt >= G.opts.num_pkts_expected)
      ti->inl_is_stopped = 1;
    if (G.expire_cyc && mal_get_cycles() >= G.expire_cyc)
      ti->inl_is_stopped = 1;
  }
  MAL_MUTEX_UNLOCK(&ti->mtx);

  /* Finally zero out our temp stats */
  tstats->rx_app_cnt = 0;
  tstats->tx_app_cnt = 0;
  tstats->rfl_app_cnt = 0;
  tstats->tx_app_eagain = 0;
  tstats->tx_app_dropped = 0;
}

void
inl_finalize_stats(struct inl_worker_info *w, struct inl_stats *tstats)
{
  inl_update_stats(w, tstats);
  snf_inject_getstats(w->injh, &w->istats);
  snf_ring_getstats(w->ringh, &w->rstats);
}

/* reflect packet to netdev iff it's not a TCP or UDP packet */
int
handle_netdev_reflect(snf_netdev_reflect_t handle, const void *buf, uint32_t len,
                      int *reflected)
{
  uint16_t ethtype;
  uint8_t ip_proto;
  uint8_t *fp = (uint8_t *) buf;
  int rc = 0;
  *reflected = 0;

  fp += 12; /* skip dst, src mac */
  ethtype = htons(*(uint16_t *)fp);
  if (ethtype == 0x8100) { /* skip VLAN */
    fp += 4;
    ethtype = htons(*(uint16_t *)fp);
  }
  fp += 2; /* skip ethtype */

  ip_proto = 0; /* assume non-TCP/UDP */
  if (ethtype == 0x0800) { /* IPv4 */
    ip_proto = *(fp + 9 /* Protocol field */);
  } else if (ethtype == 0x86dd) { /* IPv6 */
    ip_proto = *(fp + 6 /* Next Header field */);
  }

  if ((ip_proto != 6) && (ip_proto != 17)) {
    rc = snf_netdev_reflect(handle, buf, len);
    if (rc == 0) {
      *reflected = 1;
    }
  }
  return rc;
}

MAL_THREAD_RETURN_T
inl_worker(void *data)
{
  struct inl_worker_info *w = (struct inl_worker_info *)data;
  struct inl_info *ti = w->ti;
  struct snf_recv_req recv_req;
  int timeout_recv_ms = 200;
  int rc;
  int tx_tries;
  int do_stats_update = 0;
  struct inl_stats stats;
  int reflected;

#ifndef MAL_KERNEL
#ifndef PTHREAD_SETAFFINITY_NP_MISSING
  if (w->cpu_id >= 0) /* ... and setaffinity_np is available */
  {
    unsigned cpu = w->cpu_id;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), (void *)&cpuset);
  }
#endif
  {
    sigset_t sigmask;
    sigfillset(&sigmask);
#if !defined __FreeBSD__
    sigdelset(&sigmask, SIGINT);
#endif
    pthread_sigmask(SIG_BLOCK, &sigmask, NULL);
  }
#endif /* MAL_KERNEL */

  SNF_Safe( snf_inject_open(ti->board_tx, 0, &w->injh) );
  SNF_Safe( snf_ring_open_id(ti->snfh, w->worker_id, &w->ringh) );

  MAL_PRINT(("myri_snf_kinl %d.%d> from board %d to board %d, running on CPU %d\n", 
      ti->inl_id, w->worker_id, ti->board_rx, ti->board_tx, 
      w->cpu_id));

  bzero(&stats, sizeof(stats));

  while (!ti->inl_is_stopped) {
    if (do_stats_update) {
      inl_update_stats(w, &stats);
      do_stats_update = 0;
    }
    rc = snf_ring_recv(w->ringh, timeout_recv_ms, &recv_req);

    if (rc == 0) {
      if (++stats.rx_app_cnt >= 500000)
        do_stats_update = 1;

      /* TODO: Insert fake delay loop here */

      /* Reflect to netdev if reflection is enabled and packet matches, else reinject */
      reflected = 0;
      if (DBG_NETDEV_REFLECT_ALL || G.netdev_reflect) {
        SNF_Safe(handle_netdev_reflect(ti->reflect_handle, recv_req.pkt_addr, recv_req.length,
                 &reflected));
        if (reflected) {
          ++stats.rfl_app_cnt;
        }
      }

      if (! reflected) {
        do {
          rc = snf_inject_send(w->injh, G.opts.tx_timeout_ms, recv_req.pkt_addr, recv_req.length);
        } while (rc == EINTR);
        if (rc == 0) {
tx_success:
          ++stats.tx_app_cnt;
          continue;
        }
        if (rc == EAGAIN) {
          if (G.opts.tx_timeout_ms > 0) {
            stats.tx_app_eagain++;
            stats.tx_app_dropped++;
            continue;
          }
          stats.tx_app_eagain++;
          tx_tries = G.opts.tx_num_tries;
          while (rc == EAGAIN && tx_tries--) {
            do {
              rc = snf_inject_send(w->injh, 0, recv_req.pkt_addr, recv_req.length);
            } while (rc == EINTR);
          }
          if (rc == 0)
            goto tx_success;
          stats.tx_app_dropped++;
          if (rc == EAGAIN)
            continue;
        }
        SNF_Safe(rc);
      }
    }
    else if (rc == EINTR || rc == EAGAIN) { /* from snf_ring_recv */
      if (stats.rx_app_cnt)
        do_stats_update = 1;
      continue;
    }
    else
      SNF_Safe(rc);
  }

  inl_finalize_stats(w, &stats);

  SNF_Safe( snf_inject_close(w->injh) );
  SNF_Safe( snf_ring_close(w->ringh) );

bail:
  MAL_THREAD_EXIT_WITH_NOTHING(w->wtask);
  return (MAL_THREAD_RETURN_T)(uintptr_t) rc;
}

static int
cpu_getmask_ith(unsigned cpumask, int id)
{
  int i, i_found = 0;
  unsigned mask;
  for (i = 0; i < sizeof(cpumask)*8; i++) {
    mask = (1U<<i);
    if (mask & cpumask) {
      if (i_found == id) /* cpu i to be used for id */
        return i;
      i_found++;
    }
  }
  /* not found, return any cpu */
  return -1;
}

static struct inl_options opts_default = 
  { .verbose = 0,
    .tx_num_tries = 80000,
    .tx_timeout_ms = 0,
    .num_pkts_expected = 0
  };

void
inl_init(struct inl_options *opts)
{
  snf_init(SNF_VERSION_API);
  if (opts == NULL)
    G.opts = opts_default;
  else
    G.opts = *opts;
}

int
inl_num_devices(int *numdev)
{
  struct snf_ifaddrs *ifaddrs = NULL, *ifa;
  int num = 0;
  int rc;

  SNF_Safe( snf_getifaddrs(&ifaddrs) );
  ifa = ifaddrs;
  while (ifa != NULL) {
    num++;
    ifa = ifa->snf_ifa_next;
  }

bail:
  if (ifaddrs)
    snf_freeifaddrs(ifaddrs);
  *numdev = num;
  return rc;
}

int
inl_add(int board_rx, int board_tx, int num_rings, unsigned cpumask)
{
  struct inl_info *ti;
  int id, numdev;

  if (inl_num_devices(&numdev)) {
    MAL_PRINT(("Can't obtain any SNF devices\n"));
    return -1;
  }
  else {
    if (board_rx >= numdev) {
      MAL_PRINT(("RX board %d is invalid\n", board_rx));
      return -1;
    }
    if (board_tx >= numdev) {
      MAL_PRINT(("TX board %d is invalid\n", board_tx));
      return -1;
    }
  }
  if (G.num_inls == MAX_INLINES) {
    MAL_PRINT(("Invalid number of inls, max is %d\n", MAX_INLINES));
    return -1;
  }
  id = G.num_inls;
  ti = &G.inl[id];

  ti->inl_id = id;
  ti->inl_is_stopped = 0;
  ti->board_rx = board_rx;
  ti->board_tx = board_tx;
  ti->nrings = num_rings;
  ti->cpumask = cpumask;
  MAL_MUTEX_INIT(&ti->mtx);
  G.num_inls++;
  return 0;
}

static int
start_inl(int inl_id)
{
  struct inl_info *ti;
  struct inl_worker_info *wi;
  int i, rc;
  char buf[96];

  ti = &G.inl[inl_id];

#ifndef PTHREAD_SETAFFINITY_NP_MISSING
  if (ti->cpumask) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (i = 0; i < sizeof(ti->cpumask) * 8; i++) {
      if (ti->cpumask & (1<<i))
        CPU_SET(i, &cpuset);
    }
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), (void *)&cpuset);
  }
#endif

  SNF_Safe( snf_open(ti->board_rx, ti->nrings, NULL, 0, -1, &ti->snfh) );

  if (G.netdev_reflect)
    SNF_Safe( snf_netdev_reflect_enable(ti->snfh, &ti->reflect_handle) );

  for (i = 0; i < ti->nrings; i++) {
    snprintf(buf, sizeof buf - 1, "snf_inline.%d.%d", ti->board_rx, i);
    wi = &ti->winfo[i];
    bzero(wi, sizeof(*wi));
    wi->ti = ti;
    wi->rc = 0;
    wi->worker_id = i;
    wi->cpu_id = cpu_getmask_ith(ti->cpumask, i);
    MAL_THREAD_CREATE_BIND(&wi->wtask, inl_worker, (void *) wi, buf, wi->cpu_id);
  }

bail:
  return rc;
}

int
inl_start(void)
{
  int inl_id, rc = 0;
  for (inl_id = 0; inl_id < G.num_inls; inl_id++) {
    if ((rc = start_inl(inl_id)))
      return rc;
    SNF_Safe( snf_start(G.inl[inl_id].snfh) );
    G.inl[inl_id].inl_is_started = 1;
  }
bail:
  return rc;
}

void
inl_stop(void)
{
  int inl_id;
  for (inl_id = 0; inl_id < G.num_inls; inl_id++)
    if (G.inl[inl_id].inl_is_started)
      G.inl[inl_id].inl_is_stopped = 1;
}

void
inl_wait(void)
{
  int inl_id, w_id;
  int rc;
  struct inl_worker_info *wi;
  for (inl_id = 0; inl_id < G.num_inls; inl_id++) {
    if (!G.inl[inl_id].inl_is_started)
      continue;
    for (w_id = 0; w_id < G.inl[inl_id].nrings; w_id++) {
      wi = &G.inl[inl_id].winfo[w_id];
      MAL_THREAD_JOIN(&wi->wtask);
    }
  }
  for (inl_id = 0; inl_id < G.num_inls; inl_id++)
    SNF_Safe( snf_close(G.inl[inl_id].snfh) );
bail:
  return;
}

void
dump_inl_stats(int id)
{
  struct inl_info *ti = &G.inl[id];
  struct inl_worker_info *wi;
  struct inl_worker_info *wi0;
  int i;

  wi0 = &G.inl[id].winfo[0];

  MAL_PRINT(("\nTap %d:      nic_rx=%10llu   nic_tx=%10llu phy_rx_drop=%10llu snf_rx_drop=%10llu\n",
      id,
      (unsigned long long) wi0->rstats.nic_pkt_recv,
      (unsigned long long) wi0->istats.nic_pkt_send,
      (unsigned long long) wi0->rstats.nic_pkt_overflow,
      (unsigned long long) wi0->rstats.snf_pkt_overflow));

  for (i = 0; i < ti->nrings; i++) {
    wi = &G.inl[id].winfo[i];
    MAL_PRINT(("\t[%d] lib_rx=%10llu   nic_tx=%10llu lib_rx_drop=%10llu,     netdev=%10llu\n", i,
      (unsigned long long) wi->rstats.ring_pkt_recv,
      (unsigned long long) wi->istats.inj_pkt_send,
      (unsigned long long) wi->rstats.ring_pkt_overflow,
      (unsigned long long) wi->wstats.rfl_app_cnt));
  }

  for (i = 0; i < ti->nrings; i++) {
    wi = &G.inl[id].winfo[i];
    MAL_PRINT(("\t[%d] app_rx=%10llu   app_tx=%10llu   tx_eagain=%10llu,    tx_drop=%10llu\n", i,
      (unsigned long long) wi->wstats.rx_app_cnt,
      (unsigned long long) wi->wstats.tx_app_cnt,
      (unsigned long long) wi->wstats.tx_app_eagain,
      (unsigned long long) wi->wstats.tx_app_dropped));
  }
}

void
inl_dump_stats(void)
{
  int i;
  for (i = 0; i < G.num_inls; i++)
    dump_inl_stats(i);
}

#ifndef MAL_KERNEL
static MAL_MUTEX_T  exit_mtx;
void
sigexit(int sig)
{
  int do_exit;
  static enum { NONE, WAITING_INT } wait_state = NONE;
  
  do_exit = (sig < 0);

  MAL_MUTEX_LOCK(&exit_mtx);
  if (wait_state == NONE && sig == SIGINT) {
      int i, w;
      /* try to cleanly make threads exit */
      for (i = 0; i < G.num_inls; i++) {
        G.inl[i].inl_is_stopped = 1;
        wait_state = WAITING_INT;
        for (w = 0; w < G.inl[i].nrings; w++)
          if (G.inl[i].winfo[w].wtask)
            pthread_kill((pthread_t) G.inl[i].winfo[w].wtask, SIGINT);
      }
      wait_state = WAITING_INT;
      alarm(3);
      do_exit = 0;
  }
  else if (wait_state == WAITING_INT && sig != SIGALRM && sig >= 0)
    do_exit = 0; /* wait for sigalrm */
  MAL_MUTEX_UNLOCK(&exit_mtx);

  if (do_exit) {
    inl_dump_stats();
    exit(0);
  }
}

void
usage(const char *progname)
{
  printf("usage: %s [options] -b <...> [ -b <...> ... ]\n\n", progname);
  printf(" options:\n");
  printf(" -b <board_source>:<board_dest>:<num_rings>[:<0xcpumask>]\n");
  printf("    <board_source> is where to capture packets\n");
  printf("    <board_dest>   is where to forward packets\n");
  printf("    <num_rings>    is the number of rings/workers to dedicate to capture\n");
  printf("    <cpumask>      is an optional binding cpumask in hexadecimal\n");
  printf("\n");
  printf("    -n <num_packets>: Number of packets to forward before exiting\n");
  printf("   -N <tx_try_again>: Number of times to try injecting before dropping packet\n");
  printf("  -W <tx_wait_msecs>: Number of milleseconds to wait in snf_inject_send\n");
  printf("                   R: Reflect non UDP and TCP packets to network device\n");
  printf("\n");
}

int
main(int argc, char **argv)
{
  char c;
  int rc = 0, i;
  int ninls = 0;
  struct inl_options opt;
  struct inl_add_info {
    int board_rx;
    int board_tx;
    int nrings;
    unsigned cpumask;
  } inls[MAX_INLINES];

  opt = opts_default;
  G.netdev_reflect = 0;

  snf_init(SNF_VERSION_API);

  MAL_MUTEX_INIT(&exit_mtx);

  while ((c = getopt(argc, argv, "b:v:N:W:n:R")) != -1) {
    if (c == 'b') {
      if (ninls == MAX_INLINES) {
        printf("Invalid number of inls, max is %d\n", MAX_INLINES);
        return -1;
      }
      inls[ninls].cpumask = (~0U);
      if (sscanf(optarg, "%u:%u:%u:%x", 
                 &inls[ninls].board_rx,
                 &inls[ninls].board_tx,
                 &inls[ninls].nrings,
                 &inls[ninls].cpumask) < 3) {
        printf("Invalid Inline device <boardid_source:boardid_dest:num_rings:cpumask>\n");
        return -1;
      }
      ninls++;
    } else if (c == 'v') {
      opt.verbose++;
    } else if (c == 'N') { 
      opt.tx_num_tries = strtoul(optarg, NULL, 0);
    } else if (c == 'W') { 
      opt.tx_timeout_ms = strtoul(optarg, NULL, 0);
    } else if (c == 'n') { 
      opt.num_pkts_expected = strtoull(optarg, NULL, 0);
    } else if (c == 'R') { 
      G.netdev_reflect = 1;
    } else {
      if (c != 'h')
        printf("ERROR: Unknown option: %c\n", c);
      usage(argv[0]);
      return -1;
    }
  }

  if (ninls == 0) {
    printf("ERROR: No inls requested\n");
    usage(argv[0]);
    return -1;
  }

  inl_init(&opt);
  for (i = 0; i < ninls; i++) {
    if (inl_add(inls[i].board_rx, inls[i].board_tx, inls[i].nrings, inls[i].cpumask)) {
      printf("Could not initialize inline deviec %d->%d with %d rings\n",
          inls[i].board_rx, inls[i].board_tx, inls[i].nrings);
    }
  }

  if (SIG_ERR == signal(SIGINT, sigexit))
    exit(1);
  if (SIG_ERR == signal(SIGTERM, sigexit))
    exit(1);
  if (SIG_ERR == signal(SIGALRM, sigexit))
    exit(1);

  if ((rc = inl_start()))
    return rc;

  inl_wait(); /* stop request comes from signal handler or event */

  inl_dump_stats();

  return rc;
}
#endif
