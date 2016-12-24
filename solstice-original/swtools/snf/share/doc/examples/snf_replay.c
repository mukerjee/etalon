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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#define __USE_GNU
#include <pthread.h>
#include <signal.h>

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

#include "pcap.h"
#include "snf.h"

static int verbose = 0;
static uint64_t num_packets = -1;
static int num_threads = 1;
static const char *cap_file = NULL;
static int boardnum = 0;
static int iters = 1;

#define MAX_THREADS 16

struct inj_thread_info {
  uint32_t   id;
  uint64_t   pkts_cnt;
  uint64_t   bytes_cnt;
  uint64_t   t_start_us;
  uint64_t   t_stop_us;
  uint64_t   nerror;
  uint8_t   pad[64-sizeof(uint32_t)-3*sizeof(uint64_t)];
} inj_info[MAX_THREADS];

void
usage(void)
{
  printf("Usage: snf_replay [-v] [-b <board>] [-n <count>] [-t <nthreads>] <file.pcap>\n");
  printf("           -v: verbose\n");
  printf("   -b <board>: Myri10G board number to use.\n");
  printf("   -n <count>: Maximum number of packets to send from pcap file\n");
  printf("   -t <nthreads>: Number of threads to concurrently send same file\n");
  printf("   -i <iters>: Number of times to replay the file on each thread\n");
  exit(1);
}

void * work_thread(void *arg);

#define SNF_Safe(x) do {      \
  int ret = (x);              \
  if (ret != 0) {                                               \
    fprintf(stderr, "SNF Failure at line %d: %s (errno=%d)\n",  \
        __LINE__, strerror(ret), ret);                          \
    exit(1); \
  } } while (0)

void
print_tinfo(struct inj_thread_info *tinfo, const char *prefix)
{
  static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
  uint64_t t_usec;
  pthread_mutex_lock(&mtx);
  t_usec = tinfo->t_stop_us - tinfo->t_start_us;
  printf("%s> Packets: %lld\n", prefix, (long long) tinfo->pkts_cnt);
  printf("%s> Bytes: %lld\n", prefix, (long long) tinfo->bytes_cnt);
  printf("%s> Rate: %.2f Mpps\n", prefix, (double) (tinfo->pkts_cnt) / t_usec);
  printf("%s> Throughput: %.3f Gbps in %.3f secs\n", prefix,
      (double) (tinfo->bytes_cnt * 8 / 1e9) / (t_usec / 1e6),
      (double) t_usec / 1e6);
  if (tinfo->nerror)
    printf("%s> inject errors: %lld\n", prefix, (long long) tinfo->nerror);
  pthread_mutex_unlock(&mtx);
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

int
main(int argc, char **argv)
{
  int rc, i;
  char c;
  pthread_t thread_ids[MAX_THREADS];
  struct inj_thread_info tinfo;
  void *thread_rc;

  /* get args */
  while ((c = getopt(argc, argv, "vb:n:t:i:")) != -1) {
    if (c == 'v') {
      verbose++;
    } else if (c == 'b') {
      boardnum = strtol(optarg, NULL, 0);
    } else if (c == 'n') {
      num_packets = (uint64_t) strtoull(optarg, NULL, 0);
    } else if (c == 't') {
      num_threads = (int) strtol(optarg, NULL, 0);
      if (num_threads > MAX_THREADS) {
	fprintf(stderr, "Invalid number of threads (%d, max:%d)\n",
		num_threads, MAX_THREADS);
        exit(1);
      }
    } else if (c == 'i') {
      iters = (int) strtol(optarg, NULL, 0);
    } else {
      printf("Unknown option: %c\n", c);
      usage();
    }
  }

  if (argc-optind < 1)
    usage();

  cap_file = argv[optind];

  snf_init(SNF_VERSION_API);

  memset(inj_info, 0, sizeof(inj_info));
  for (i = 0; i < num_threads; i++) {
    inj_info[i].id = i;
    rc = pthread_create(&thread_ids[i], NULL, work_thread, &inj_info[i]);
    if (rc != 0) {
      perror("pthread_create");
      exit(1);
    }
  }

  tinfo.pkts_cnt = tinfo.bytes_cnt = 0;
  tinfo.t_start_us = -1;
  tinfo.t_stop_us = 0;
  tinfo.nerror = 0;
  for (i = 0; i < num_threads; i++) {
    rc = pthread_join(thread_ids[i], &thread_rc);
    if (rc == 0 && thread_rc) {
      struct inj_thread_info *ti = (struct inj_thread_info *)thread_rc;
      tinfo.pkts_cnt += ti->pkts_cnt;
      tinfo.bytes_cnt += ti->bytes_cnt;
      if (ti->t_start_us < tinfo.t_start_us)
        tinfo.t_start_us = ti->t_start_us;
      if (ti->t_stop_us > tinfo.t_stop_us)
        tinfo.t_stop_us = ti->t_stop_us;
    }
  }

  if (tinfo.pkts_cnt && num_threads > 1) {
    printf("\nAggregate packet send information for %d threads:\n", num_threads);
    print_tinfo(&tinfo, "All threads");
  }

  return 0;
}

void *
work_thread(void *arg)
{
  struct inj_thread_info *ij = (struct inj_thread_info *)arg;
  pcap_t *p;
  char errbuf[PCAP_ERRBUF_SIZE];
  char pbuf[64];
  struct pcap_pkthdr pkthdr;
  const u_char *pdata;
  int rc, i, done;
  snf_inject_t hinj;

#ifndef PTHREAD_SETAFFINITY_NP_MISSING
  {
    unsigned cpu = ij->id % 8;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), (void *)&cpuset);
  }
#endif

  snprintf(pbuf, sizeof(pbuf) - 1, "Thread %d", ij->id);

  SNF_Safe( snf_inject_open(boardnum, 0, &hinj) );

  done = 0;
  for (i = 0; i < iters; i++) {
    if (!(p = pcap_open_offline(cap_file, errbuf))) {
      fprintf(stderr, "%s: Error opening pcap file: %s\n", pbuf, errbuf);
      return NULL;
    }

    if (i == 0)
      ij->t_start_us = test_gettimeofday_usec();

    while ((pdata = pcap_next(p, &pkthdr)) != NULL) { 
      if (!(rc = snf_inject_send(hinj, 1000, (void *) pdata, pkthdr.caplen))) {
        ij->pkts_cnt++;
        ij->bytes_cnt += pkthdr.caplen;
      }
      else if (rc != EAGAIN) {
        perror("snf_inject_packet unhandled error");
        break;
      }
      else
        ij->nerror++;

      if (num_packets != -1 && ((int) ij->pkts_cnt == num_packets)) {
        done = 1;
        break;
      }
    }
    if (i == iters-1 || done)
      ij->t_stop_us = test_gettimeofday_usec();

    pcap_close(p);
  }

  SNF_Safe( snf_inject_close(hinj) );

  print_tinfo(ij, pbuf);
  return ij;
}
