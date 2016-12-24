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

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#define __USE_GNU
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "snf.h"

#ifdef __FreeBSD__
#include <sys/param.h>
#if __FreeBSD_version < 702100
#define PTHREAD_SETAFFINITY_NP_MISSING
#else
#include <pthread_np.h>
#define cpu_set_t cpuset_t
#endif /* __FreeBSD_version */
#elif defined(__GLIBC__) && !__GLIBC_PREREQ(2,4)
#define PTHREAD_SETAFFINITY_NP_MISSING
#endif
#define MAX_WORKERS               32
#define MAX_CONC_RECVS            1024

#define DBG1(vargs...) if (G.verbose) printf(vargs)
#define DBG2(vargs...) if (G.verbose > 1) printf(vargs)
#define DBG3(vargs...) if (G.verbose > 2) printf(vargs)

static inline int pkt_validate(void *pkt, uint32_t length);

#define TV_TO_US(tv) ((tv)->tv_sec * 1000000 + (tv)->tv_usec)

struct worker {
  int           worker_id;
  int           cpu_id;
  snf_ring_t    hring;
  snf_inject_t  hinj;
  pthread_t thread_id;
  uint64_t num_pkts;
  uint64_t num_bytes;
  uint64_t num_pkts_last;
  uint64_t num_bytes_last;
  uint64_t num_pkts_inject;
};
typedef struct worker worker_t;

struct G {
  snf_handle_t hsnf;
  unsigned snap_print;
  uint64_t num_pkts;
  uint64_t num_bytes;
  uint64_t num_pkts_expected;
  uint64_t exit_usecs;
  unsigned duration;
  unsigned custom_hash_rr;
  int64_t  delay_nsecs;
  int64_t  delay_iters;
  unsigned nreqs;
  unsigned validate;
  unsigned verbose;
  int      recv_wait;
  int      reinject;
  int      copyout;
  unsigned num_workers;
  unsigned num_workers_ready;
  unsigned expired;
  uint64_t cpumask;
  worker_t worker[MAX_WORKERS];
  pthread_mutex_t wrk_mtx;
  pthread_cond_t  wrk_cond;
  struct timeval tv_last;
  uint64_t usecs_last;
  uint64_t num_pkts_last;
  uint64_t num_bytes_last;
} G;
  
struct pkt_hdr {
  uint32_t length;
  uint32_t ofst;
};
typedef struct pkt_hdr pkt_hdr_t;

static void
usage(const char *progname)
{
  printf("usage: %s -w <num_workers> [options]\n\n", progname);
  printf(" options:\n");
  printf(" -b <board number>: Myri10G board number to use.\n");
  printf("     -n <num pkts>: number of packet to receive (default: 0 - infinite)\n");
  printf("                -t: show periodic stats, every second\n");
  printf("                -V: validate incoming packets\n");
  printf("                -v: verbose, or -vv verry verbose\n");
  printf("      -d <ring sz>: size of the receive ring in bytes, or megabyte\n");
  printf("     -D <nanosecs>: add <nanosecs> of synthetic processing delay in packet handling\n");
  printf("        -W <msecs>: timeout of <msecs> in blocking receive calls\n");
  printf("     -S <snap len>: display first <snap len> bytes of each packet\n");
  printf(" -R <board number>: reinject every received packet on board <boardnum>\n");
  printf("\n");
  exit(1);
}

static void
dump_data(unsigned char *pg, int len)
{
  int i = 0;
  while (i < len) {
    printf
      ("%04d: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
       i, pg[i], pg[i + 1], pg[i + 2], pg[i + 3], pg[i + 4], pg[i + 5],
       pg[i + 6], pg[i + 7], pg[i + 8], pg[i + 9], pg[i + 10], pg[i + 11],
       pg[i + 12], pg[i + 13], pg[i + 14], pg[i + 15]);
    i += 16;
  }
}

static inline int
pkt_validate(void *pkt, uint32_t length)
{
  pkt_hdr_t *hdr = (pkt_hdr_t *)pkt;
  uint32_t *pld_ptr;
  uint32_t hdr_ofst = ntohl(hdr->ofst) / 4;
  uint32_t i;

  if (length != ntohl(hdr->length)) {
    printf("packet length missmatch, in mesg:%u, in packet:%u\n",
            length, hdr->length);
    dump_data((unsigned char *)pkt, 32);
    return 1;
  }

  pld_ptr = &((uint32_t *)pkt)[2];
  for (i = 0; i < length / 4 - 2; i++) {
    if (*pld_ptr != hdr_ofst) {
      printf("packet data missmatch at ofst %u, expected:%u, actual:%u\n",
            i, hdr_ofst, *pld_ptr);
      dump_data((unsigned char *)pkt, length);
      return 1;
    }
    pld_ptr++;
    hdr_ofst++;
  }
  return 0;
}

int64_t
test_gettimeofday_usec(void)
{
  struct timeval tv;
  int64_t usec;
  gettimeofday(&tv, NULL);
  usec = (int64_t) tv.tv_sec * 1000000;
  usec += (int64_t) tv.tv_usec;
  return usec;
}

double
delay_iters(int64_t n)
{
  static volatile double x, y;
  static volatile double z = 1.000001;

  int64_t i;

  y = z;
  x = 1.0;

  for (i = 0; i < n; i++)
    x *= y;
  return x;
}

/*
 * Run delay_iters() as many times as it takes to cover at least *time_p
 * microseconds.  Returns the number of iterations to call delay_iters() with
 * to delay at least *time_p microseconds.
 */
static
int64_t delay_iters_usecs(int iters, int64_t *time_p)
{
  int64_t t_begin, t_dur, t_target = *time_p;
  const int64_t delay_loop_min = 100;
  int delay_calibration_limit = 100;
  int64_t nloops = 0;
  int i, ncalibs = 0;
  double ratio = 0.0;

  do {
    if (!nloops)
      nloops = delay_loop_min;
    else {
      int64_t tmp = nloops * ratio;
      if (tmp > nloops)
        nloops = tmp;
      else
        nloops++;
      assert(nloops < 1ll<<62);
    }
    t_begin = test_gettimeofday_usec();
    for (i = 0; i < iters; i++)
      delay_iters(nloops);
    t_dur = test_gettimeofday_usec() - t_begin;
    if (t_dur == 0)
      ratio = 2.0;
    else
      ratio = (double) t_target / t_dur;
    if (++ncalibs > delay_calibration_limit) {
      fprintf(stderr, "Couldn't converage delay_iters loop after %d iters\n", 
              iters);
      return 0;
    }
  } while (ratio > 1.0);

  *time_p = t_dur;
#if 0
  t_begin = test_gettimeofday_usec();
  for (i = 0; i < 1000; i++)
    delay_iters(100);
  t_dur = test_gettimeofday_usec() - t_begin;
  printf("Each 100 iters costs %.3f usecs\n",
      (double) t_dur / 1000.0);
#endif
  return nloops;
}

void
stats(void)
{
  worker_t *wrk;
  unsigned int w;
  uint64_t num_pkts = 0;
  int64_t num_bytes = 0;
  int have_nic_stats = 0;
  int have_nic_stats_inj = 0;
  struct snf_ring_stats stats;
  struct snf_inject_stats istats;
  for (w = 0; w < G.num_workers; w++) {
    wrk = &G.worker[w];
    num_pkts += wrk->num_pkts;
    num_bytes += wrk->num_bytes;
    if (wrk->hring == NULL || snf_ring_getstats(wrk->hring, &stats)) {
      printf("[%u] %" PRIu64 " pkts, %" PRIu64" bytes\n", w, wrk->num_pkts,
         wrk->num_bytes);
    }
    else {
      have_nic_stats = 1;
      printf("Ring %d Packets received,    ring:%12" PRIu64 ", app:%12" PRIu64 
             ", overflow:%12" PRIu64 "\n", w, stats.ring_pkt_recv, wrk->num_pkts,
             stats.ring_pkt_overflow);
    }
    if (wrk->hinj && 0 == snf_inject_getstats(wrk->hinj, &istats)) {
      printf("Ring %d Packets reinjected,  ring:%12" PRIu64 "\n", w,
          istats.inj_pkt_send);
      have_nic_stats_inj = 1;
    }
  }

  if (have_nic_stats) {
    /* NIC-level stats */
    printf("SNF dropped overflow SW: %12" PRIu64 "\n", stats.snf_pkt_overflow);
    printf("NIC dropped overflow HW: %12" PRIu64 "\n", stats.nic_pkt_overflow);
    printf("NIC dropped bad PHY/CRC: %12" PRIu64 "\n", stats.nic_pkt_bad);
  }
  if (have_nic_stats_inj)
    printf("NIC send packets       : %12" PRIu64 "\n\n", istats.nic_pkt_send);

  if (num_pkts) {
    printf("\n");
    printf("Total packets:                app:%12" PRIu64 "\n", num_pkts);
    printf("Average Packet Length:        app:%12" PRIu64 " bytes\n\n",
            num_bytes / num_pkts);
    printf("Total bytes:                  app:%12" PRIu64 " (%" PRIu64 " MB)\n",
            num_bytes, num_bytes / 1024 / 1024);
    num_bytes = stats.nic_bytes_recv;
    num_bytes -= (8 /* HW header */ + 4 /* CRC */) * stats.nic_pkt_recv;
    if (num_bytes > 0)
      printf("Total raw bytes + HW align    nic:%12" PRIu64 " (%" PRIu64 " MB)\n",
             num_bytes, num_bytes / 1024 / 1024);
  }
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

void
print_periodic_stats(void)
{
  float delta_secs;
  uint64_t delta_pkts;
  uint64_t delta_bytes;
  uint32_t pps;
  float gbps;
  float bps;
  struct timeval tv_now;
  uint64_t usecs_now;
  worker_t *wrk;
  unsigned int w;
  uint64_t wrk_delta_pkts[MAX_WORKERS];

  gettimeofday(&tv_now, NULL);
  usecs_now = TV_TO_US(&tv_now);
  delta_secs = (usecs_now - G.usecs_last) / 1000000.0;
  G.usecs_last = usecs_now;
  G.tv_last = tv_now;

  pthread_mutex_lock(&G.wrk_mtx);

  delta_pkts = G.num_pkts - G.num_pkts_last;
  delta_bytes = G.num_bytes - G.num_bytes_last;
  G.num_pkts_last = G.num_pkts;
  G.num_bytes_last = G.num_bytes;
  for (w = 0; w < G.num_workers; w++) {
    wrk = &G.worker[w];
    wrk_delta_pkts[w] = wrk->num_pkts - wrk->num_pkts_last;
    wrk->num_pkts_last = wrk->num_pkts;
  }

  if (G.duration && G.expired == 0) {
    if (!G.exit_usecs)
      G.exit_usecs = usecs_now + G.duration * 1000000;
    else if (usecs_now >= G.exit_usecs) {
      G.expired = 1;
    }
  }
  pthread_mutex_unlock(&G.wrk_mtx);

  if (delta_pkts) {
    pps = delta_pkts / delta_secs;
    bps = ((float) delta_bytes * 8) / delta_secs;
    gbps = bps / 1000000000.0;
  
    for (w = 0; w < G.num_workers; w++) {
      DBG1("[%u] %" PRIu64 " ", w, wrk_delta_pkts[w]);
    }

    printf
        ("%" PRIu64 " pkts (%" PRIu64 "B) in %.3f secs (%u pps), Avg Pkt: %"
         PRIu64 ", BW (Gbps): %6.3f\n",
         delta_pkts, delta_bytes, delta_secs,
         pps, delta_bytes / delta_pkts, gbps);
    fflush(stdout);

  }
}

void
sigexit(int sig)
{
  static pthread_mutex_t exit_mtx = PTHREAD_MUTEX_INITIALIZER;
  static int once = 0;

  pthread_mutex_lock(&exit_mtx);
  if (!once) {
    once = 1;
    stats();
    fflush(stdout);
  }
  pthread_mutex_unlock(&exit_mtx);
  if (sig >= 0)
    exit(0);
}

void
sigalrm(int sig)
{
  print_periodic_stats();
  alarm(1);
}

void *
work_thread(void *stuff)
{
  int worker_id;
  worker_t *wrk;
  int rc;
  struct snf_recv_req recv_req;
  sigset_t sigmask;
  uint64_t num_pkts = 0;
  void *copybuf = NULL;

  wrk = (worker_t *)stuff;
  worker_id = wrk->worker_id;

#ifndef PTHREAD_SETAFFINITY_NP_MISSING
  if (wrk->cpu_id >= 0)
  {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(wrk->cpu_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), (void *)&cpuset);
    printf("Worker %d running on CPU %d\n", worker_id, wrk->cpu_id);
  }
#endif

  /* mask signals except SIGINT to allow ^C */
  sigfillset(&sigmask);
#if !defined __FreeBSD__
  sigdelset(&sigmask, SIGINT);
#endif
  rc = pthread_sigmask(SIG_BLOCK, &sigmask, NULL);

  wrk->hring = NULL;
  rc = snf_ring_open(G.hsnf, &wrk->hring);
  if (rc) {
    fprintf(stderr, "[%d] Can't open ring %d: %s\n", worker_id, worker_id, strerror(rc));
    goto done_thread;
  }
  if (G.reinject >= 0) {
    rc = snf_inject_open(G.reinject, 0, &wrk->hinj);
    if (rc) {
      fprintf(stderr, "[%d] Can't inject on ring %d\n", worker_id, G.reinject);
      goto done_thread;
    }
  }
  if (G.copyout) {
    copybuf = calloc(1, 16*1024);
    if (copybuf == NULL) {
      fprintf(stderr, "[%d] Can't allocate copyout buffer\n", worker_id);
      goto done_thread;
    }
  }

  pthread_mutex_lock(&G.wrk_mtx);
  if (++G.num_workers_ready == G.num_workers) {
    pthread_mutex_unlock(&G.wrk_mtx);
    pthread_cond_broadcast(&G.wrk_cond);
    rc = snf_start(G.hsnf);
    if (rc) {
      fprintf(stderr, "[%d] Can't start snf: %s\n", worker_id, strerror(rc));
      goto done_thread;
    }
  }
  else {
    pthread_cond_wait(&G.wrk_cond, &G.wrk_mtx);
    pthread_mutex_unlock(&G.wrk_mtx);
  }

  DBG1("[%u] started\n", worker_id);

  while (1) {
    rc = snf_ring_recv(wrk->hring, G.recv_wait, &recv_req);
    if (rc == 0) {
	dump_data((unsigned char *) recv_req.pkt_addr, recv_req.length);
      wrk->num_bytes += recv_req.length;
      wrk->num_pkts++;
      num_pkts++;
      if (copybuf != NULL)
        memcpy(copybuf, recv_req.pkt_addr, recv_req.length);
      if (G.reinject >= 0) {
        rc = snf_inject_send(wrk->hinj, 10, recv_req.pkt_addr, recv_req.length);
        if (!rc)
          wrk->num_pkts_inject++;
      }
      DBG2("[%d] received %d bytes (so far %llu)\n", worker_id, recv_req.length,
                         (unsigned long long) wrk->num_pkts);
      if (G.snap_print > 0) {
          dump_data(recv_req.pkt_addr,
             (recv_req.length > G.snap_print) ? G.snap_print : recv_req.length);
      }
      if (G.delay_iters > 0)
        delay_iters(G.delay_iters);
      if (num_pkts >= 100000)
        goto update_num_pkts;
    }
    else if (rc == EINTR) {
      if (G.expired)
        goto update_num_pkts;
      else
        continue;
    }
    else if (rc == EAGAIN) { /* timed out */
update_num_pkts:
      pthread_mutex_lock(&G.wrk_mtx);
      G.num_pkts += num_pkts;
      num_pkts = 0;
      if (G.num_pkts >= G.num_pkts_expected || G.expired) {
        int w;
        DBG1("[%d] all %llu received\n", worker_id,
                                       (unsigned long long) G.num_pkts);
        /* kill off all the workers */
        for (w = 0; w < G.num_workers; w++) {
          if (worker_id != G.worker[w].worker_id) {
            DBG1("[%d] killing worker %d\n", worker_id, G.worker[w].worker_id);
            pthread_cancel(G.worker[w].thread_id);
          }
        }
        pthread_mutex_unlock(&G.wrk_mtx);
        break;
      }
      else {
        pthread_mutex_unlock(&G.wrk_mtx);
      }
    }
    else {
      fprintf(stderr, "[%d] unexpected snf_ring_recv error %d (%s)\n", 
          worker_id, rc, strerror(rc));
      break;
    }
  }

done_thread:
  if (copybuf)
    free(copybuf);
  DBG1("[%d] exiting\n", worker_id);
  return NULL;
}

int
main(int argc, char **argv)
{
  int rc = 0;
  char c;
  int periodic_stats = 0;
  int boardnum = 0;
  uint64_t dataring_sz = 0;
  int w;
  worker_t *wrk;
  void *thread_rc;

  /* set defaults */
  G.num_workers = 0;
  G.recv_wait = 500;
  G.num_pkts_expected = -1;
  G.duration = 0;
  G.delay_nsecs = 0;
  G.delay_iters = 0;
  G.custom_hash_rr = 0;
  G.snap_print = 0;
  G.nreqs = 1;
  G.validate = 0;
  G.verbose = 0;
  G.reinject = -1;
  G.copyout = 0;
  G.cpumask = ~0ULL;

  /* get args */
  while ((c = getopt(argc, argv, "b:tvVR:S:n:T:d:w:D:W:hC")) != -1) {
    if (c == 'b') {
      boardnum = strtoul(optarg, NULL, 0);
    } else if (c == 't') {
      periodic_stats = 1; 
    } else if (c == 'V') {
      G.validate = 1;
    } else if (c == 'n') {
      G.num_pkts_expected = strtoul(optarg, NULL, 0);
    } else if (c == 'T') {
      G.duration = strtoul(optarg, NULL, 0);
    } else if (c == 'v') {
      G.verbose++;
    } else if (c == 'R') {
      G.reinject = strtoul(optarg, NULL, 0);
    } else if (c == 'S') {
      G.snap_print = strtoul(optarg, NULL, 0); 
    } else if (c == 'd') {
      dataring_sz = strtoull(optarg, NULL, 0);
    } else if (c == 'D') {
      G.delay_nsecs = strtoll(optarg, NULL, 0);
    } else if (c == 'C') {
      G.copyout = 1;
    } else if (c == 'w') {
      long long cpumask;
      int n;
      if ((n = sscanf(optarg, "%u:%llx", &G.num_workers, &cpumask)) < 1)
        usage(argv[0]);
      if (G.num_workers > MAX_WORKERS) {
        printf("Invalid number of worker threads (%d, max:%d)\n",
            G.num_workers, MAX_WORKERS);
        exit(1);
      }
      if (n >= 2)
        G.cpumask = cpumask;
    } else if (c == 'W') {
      G.recv_wait = strtoull(optarg, NULL, 0);
    } else {
      if (c != 'h')
        printf("ERROR: Unknown option: %c\n", c);
      usage(argv[0]);
    }
  }
  if (!G.num_workers) {
    printf("\nERROR: Number of workers/rings not specified with -w option:\n\n");
    usage(argv[0]);
  }

  if (G.delay_nsecs > 0) {
    int64_t usecs = 100000, niters;
    niters = delay_iters_usecs(1, &usecs);
    if (!niters) {
      printf("Can't calibrate for delay of %d nanoseconds\n", (int) G.delay_nsecs);
      exit(1);
    }
    G.delay_iters = (int64_t)((double) niters * G.delay_nsecs / usecs / 1000.0);
    printf("After calibration, it takes %d iters to generate %d nanoseconds of delay\n",
           (int) G.delay_iters, (int) G.delay_nsecs);
  }

  snf_init(SNF_VERSION_API);

#ifndef PTHREAD_SETAFFINITY_NP_MISSING
  if (G.cpumask) {
    cpu_set_t cpuset;
    int i;
    CPU_ZERO(&cpuset);
    for (i = 0; i < sizeof(G.cpumask) * 8; i++) {
      if (G.cpumask & (1<<i))
        CPU_SET(i, &cpuset);
    }
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), (void *)&cpuset);
  }
#endif

  rc = snf_open(boardnum, G.num_workers, NULL, dataring_sz, -1, &G.hsnf);
  if (rc) {
    errno = rc;
    perror("Can't open snf");
    exit(2);
  }

  if (SIG_ERR == signal(SIGINT, sigexit))
    exit(1);
  if (SIG_ERR == signal(SIGTERM, sigexit))
    exit(1);

  pthread_mutex_init(&G.wrk_mtx, NULL);
  pthread_cond_init(&G.wrk_cond, NULL);
  G.num_workers_ready = 0;

  printf("Capture using %d rings and worker threads with default RSS hashing", G.num_workers);
  if (G.delay_nsecs)
    printf(", each thread spends an %d nanosecs of additional (synthetic) processing\n", 
        (int) G.delay_nsecs);
  else
    printf("\n");

  for (w = 0; w < G.num_workers; w++) {
    wrk = &G.worker[w];
    memset(wrk, 0, sizeof(worker_t));
    wrk->worker_id = w;
    wrk->cpu_id = cpu_getmask_ith(G.cpumask, w);

    rc = pthread_create(&wrk->thread_id, NULL, work_thread, (void *)wrk);
    if (rc != 0) {
      perror("pthread_create");
      exit(1);
    }
  }

  if (periodic_stats || G.duration) {
    if (SIG_ERR == signal(SIGALRM, sigalrm)) {
      exit(1);
    }
    gettimeofday(&G.tv_last, NULL);
    G.usecs_last = TV_TO_US(&G.tv_last);
    alarm(1);
  }

  for (w = 0; w < G.num_workers; w++)
    pthread_join(G.worker[w].thread_id, &thread_rc);

  sigexit(-1);

  for (w = 0; w < G.num_workers; w++) {
    wrk = &G.worker[w];
    if (wrk->hring) {
      snf_ring_close(wrk->hring);
      wrk->hring = NULL;
    }
    if (wrk->hinj) {
      snf_inject_close(wrk->hinj);
      wrk->hinj = NULL;
    }
  }

  rc = snf_close(G.hsnf);
  if (rc) {
    errno = rc;
    perror("Can't close snf");
  }

  exit(0);
}
