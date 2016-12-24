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

#include "/opt/snf/include/snf.h"

#define MAX_PKT_LEN	9000
#define DEF_PKT_LEN	60
#define MAX_THREADS	8

uint32_t pkt_cnt = 0;
uint32_t pkt_len = DEF_PKT_LEN;
uint32_t boardnum;

void
usage(void)
{
  printf("Usage: snf_pktgen [-v] [-b <board>] [-s <size> [-n <pkts>]\n");
  printf("           -v: verbose\n");
  printf("   -b <board>: Myri10G board number to use.\n");
  printf("    -s <size>: packet size (default: %d)\n", DEF_PKT_LEN);
  printf("    -n <pkts>: packet count (default: 0 - infinite)\n");
  printf(" -t <threads>: number of threads (max %d)\n", MAX_THREADS);
  exit(1);
}

void *
work_thread(void *arg)
{
  int i, rc, worker_id;
  sigset_t sigmask;
  snf_inject_t hinj;
  char pkt[MAX_PKT_LEN];
  
  worker_id = (int)(uintptr_t)arg;
#ifndef PTHREAD_SETAFFINITY_NP_MISSING
  {
    unsigned cpu = worker_id % 8;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), (void *)&cpuset);
  }
#endif

  /* mask signals except SIGINT to allow ^C */
  sigfillset(&sigmask);
#if !defined __FreeBSD__
  sigdelset(&sigmask, SIGINT);
#endif
  rc = pthread_sigmask(SIG_BLOCK, &sigmask, NULL);

  rc = snf_inject_open(boardnum, 0, &hinj);
  if (rc) {
    fprintf(stderr, "[%d] Can't inject on board %d: %s\n", 
	    worker_id, boardnum, strerror(rc));
    goto done_thread;
  }

  i = 0;
  while (i < pkt_cnt) {
    rc = snf_inject_send(hinj, 0, pkt, pkt_len);
    if (rc == 0) {
      i++;
    } else if (rc == EAGAIN || rc == EINTR) {
      continue;
    } else {
      fprintf(stderr, "[%d] snf_inject_send error %d (%s)\n", 
	      worker_id, rc, strerror(rc));
      break;
    }
  }

  rc = snf_inject_close(hinj);
  if (rc)
    fprintf(stderr, "[%d] Can't close inject handle %d: %s\n", 
	    worker_id, worker_id, strerror(rc));
  
done_thread:
  return NULL;
}


int
main(int argc, char **argv)
{
  int rc, i;
  char c;
  pthread_t thread_ids[MAX_THREADS];
  void *thread_rc;
  uint32_t verbose = 0, num_threads = 1;

  /* get args */
  while ((c = getopt(argc, argv, "vb:s:n:t:")) != -1) {
    if (c == 'v') {
      verbose++;
    } else if (c == 'b') {
      boardnum = strtoul(optarg, NULL, 0);
    } else if (c == 's') {
      pkt_len = strtoul(optarg, NULL, 0);
    } else if (c == 'n') {
      pkt_cnt = strtoull(optarg, NULL, 0);
    } else if (c == 't') {
      num_threads = strtoul(optarg, NULL, 0);
      if (num_threads > MAX_THREADS) {
	fprintf(stderr, "Invalid number of threads (%d, max:%d)\n",
		num_threads, MAX_THREADS);
        exit(1);
      }
    } else {
      printf("Unknown option: %c\n", c);
      usage();
    }
  }

  snf_init(SNF_VERSION_API);

  for (i = 0; i < num_threads; i++) {
    rc = pthread_create(&thread_ids[i], NULL, work_thread, (void *)(uintptr_t)i);
    if (rc != 0) {
      perror("pthread_create");
      exit(1);
    }
  }
  
  for (i = 0; i < num_threads; i++)
    pthread_join(thread_ids[i], &thread_rc);
  
  return 0;
}
