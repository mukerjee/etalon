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
 * Copyright 2008 by Myricom, Inc.  All rights reserved.
 ***********************************************************************/
/*
 * Simple program to demonstrate how to receive packets using
 * the Myricom Sniffer API.
 */

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>

#include "snf.h"

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

uint64_t num_pkts = 0;
unsigned int max_received_tv_delta = 0;
uint64_t num_bytes = 0;
uint64_t num_pkts_inject = 0;
uint64_t num_pkts_netdev_reflect = 0;
snf_ring_t hring;

#define TOGGLE(i) ((i+ 1) & 1)
#define TV_TO_US(tv) ((tv)->tv_sec * 1000000 + (tv)->tv_usec)
unsigned int itvl_idx = 0;
struct itvl_stats {
  struct timeval tv;
  uint64_t usecs;
  uint64_t num_pkts;
  uint64_t num_bytes;
} itvl[2];

float secs_delta;

void
stats()
{
  struct snf_ring_stats stats;
  uint64_t nbytes;
  int rc;
  if ((rc = snf_ring_getstats(hring, &stats))) {
    perror("nic stats failed");
  }

  printf("\n");
  if (num_pkts == stats.ring_pkt_recv) {
    printf("Packets received   in HW:        %" PRIu64 "\n", num_pkts);
  } else {
    printf("Packets received,    app:        %" PRIu64 ", ring: %" PRIu64 "\n",
           num_pkts, stats.ring_pkt_recv);
  }
  printf("Packets reinjected,    app:        %" PRIu64 "\n", num_pkts_inject);

  printf("Packets reflected to netdev:       %" PRIu64 "\n", num_pkts_netdev_reflect);

  printf("Total bytes received,  app:        %" PRIu64 " (%" PRIu64 " MB)\n",
          num_bytes, num_bytes / 1024 / 1024);
  nbytes = stats.nic_bytes_recv;
  nbytes -= (8 /* HW header */ + 4 /* CRC */) * stats.nic_pkt_recv;
  printf("Total bytes received + HW aligned: %" PRIu64 " (%" PRIu64 " MB)\n",
          nbytes, nbytes / 1024 / 1024);
  if (num_pkts > 0) {
    printf("Average Packet Length:    %" PRIu64 " bytes\n",
          num_bytes / num_pkts);
  }

  printf("Dropped, NIC overflow:    %" PRIu64 "\n", stats.nic_pkt_overflow);
  printf("Dropped, ring overflow:   %" PRIu64 "\n", stats.ring_pkt_overflow);
  printf("Dropped, bad:             %" PRIu64 "\n\n", stats.nic_pkt_bad);
}

void
print_periodic_stats(void)
{
  struct itvl_stats *this_itvl = &itvl[itvl_idx];
  struct itvl_stats *last_itvl = &itvl[TOGGLE(itvl_idx)];
  float delta_secs;
  uint64_t delta_pkts;
  uint64_t delta_bytes;
  uint32_t pps;
  float gbps;
  float bps;

  gettimeofday(&this_itvl->tv, NULL);
  this_itvl->usecs = TV_TO_US(&this_itvl->tv);
  this_itvl->num_pkts = num_pkts;
  this_itvl->num_bytes = num_bytes;
  delta_secs = (this_itvl->usecs - last_itvl->usecs) / 1000000.0;
  delta_pkts = this_itvl->num_pkts - last_itvl->num_pkts;
  delta_bytes = this_itvl->num_bytes - last_itvl->num_bytes;

  if (delta_pkts != 0) {
    pps = delta_pkts / delta_secs;
    bps = ((float) delta_bytes * 8) / delta_secs;
    gbps = bps / 1000000000.0;

    printf
      ("%" PRIu64 " pkts (%" PRIu64 "B) in %.3f secs (%u pps), Avg Pkt: %"
       PRIu64 ", BW (Gbps): %6.3f\n", delta_pkts, delta_bytes, delta_secs,
       pps, delta_bytes / delta_pkts, gbps);
    fflush(stdout);
  }

  itvl_idx = TOGGLE(itvl_idx);
}

void
sigexit(int sig)
{
  stats();
  exit(0);
}

void
sigalrm(int sig)
{
  print_periodic_stats();
  alarm(1);
}

void
usage(void)
{
  printf("Usage: snf_simple-recv [-v] [-t]  [-b <board number>] [-p] [-n <num pkts>]\n");
  printf("         [-d <ring sz>] [-S <snap len>]\n");
  printf("                  -v: verbose\n");
  printf("                  -t: print periodic statistics\n");
  printf("   -b <board number>: Myri10G board number to use.\n");
  printf("                  -p: poll for packets instead of blocking\n");
  printf("       -n <num pkts>: number of packet to receive (default: 0 - infinite)\n");
  printf("        -d <ring sz>: size if the recieve ring in bytes\n");
  printf("       -S <snap len>: display first <snap len> bytes of each packet\n");
  printf("   -R <board number>: reinject every received packet on board <boardnum>\n");
  printf("                  -N: pass every received packet to netdev\n");

  exit(1);
}

uint64_t
host_nsecs(void)
{
  uint64_t nsecs;
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  nsecs = (uint64_t) ts.tv_sec * 1000000000 + ts.tv_nsec;
  return nsecs;
}

int
main(int argc, char **argv)
{
  int rc;
  snf_handle_t hsnf;
  snf_inject_t hinj = NULL;
  snf_netdev_reflect_t hnetdev = NULL;
  struct snf_recv_req recv_req;
  char c;
  int periodic_stats = 0;
  int verbose = 0;
  int snap_print = 0;
  int inspect_pkt;
  int boardnum = 0;
  int reinject = -1;
  int netdev_reflect = 0;
  uint64_t pkts_expected = 0xffffffffffffffffULL;
  uint64_t dataring_sz = 0;
  int timeout_ms = -1;

  /* get args */
  while ((c = getopt(argc, argv, "vtb:pn:d:S:R:N")) != -1) {
    if (c == 'v') {
      verbose++;
    } else if (c == 't') {
      periodic_stats = 1; 
    } else if (c == 'b') {
      boardnum = strtoul(optarg, NULL, 0);
    } else if (c == 'p') {
      timeout_ms = 0;
    } else if (c == 'n') {
      pkts_expected = strtoul(optarg, NULL, 0);
    } else if (c == 'd') {
      dataring_sz = strtoull(optarg, NULL, 0);
    } else if (c == 'S') {
      snap_print = strtoul(optarg, NULL, 0); 
    } else if (c == 'R') {
      reinject = strtoul(optarg, NULL, 0);
    } else if (c == 'N') {
      netdev_reflect = 1;
    } else {
      printf("Unknown option: %c\n", c);
      usage();
    }
  }

  snf_init(SNF_VERSION_API);
  rc = snf_open(boardnum, 0, NULL, dataring_sz, -1, &hsnf);
  if (rc) {
    errno = rc;
    perror("Can't open snf for sniffing");
    return -1;
  }
  rc = snf_ring_open(hsnf, &hring);
  if (rc) {
    errno = rc;
    perror("Can't open a receive ring for sniffing");
    return -1;
  } 
  if (reinject >= 0) {
    rc = snf_inject_open(boardnum, 0, &hinj);
    if (rc) {
      errno = rc;
      perror("Can't re-inject packets from ring");
      return -1;
    }
  }
  if (netdev_reflect) {
    rc = snf_netdev_reflect_enable(hsnf, &hnetdev);
    if (rc) {
      errno = rc;
      perror("Can't reflect packets to netdev\n");
      return -1;
    }
  }
  rc = snf_start(hsnf);
  if (rc) {
    errno = rc;
    perror("Can't start packet capture for sniffing");
    return -1;
  }

  printf("snf_recv ready to receive\n");

  if (SIG_ERR == signal(SIGINT, sigexit))
    exit(1);
  if (SIG_ERR == signal(SIGTERM, sigexit))
    exit(1);

  if (periodic_stats) {
    if (SIG_ERR == signal(SIGALRM, sigalrm))
      exit(1);
    itvl[itvl_idx].num_pkts = 0;
    itvl[itvl_idx].num_bytes = 0;
    gettimeofday(&itvl[itvl_idx].tv, NULL);
    itvl[itvl_idx].usecs = 0;
    itvl_idx = TOGGLE(itvl_idx);
    alarm(1);
  }

  inspect_pkt = (verbose || (snap_print > 0));
  while (num_pkts < pkts_expected) {
    rc = snf_ring_recv(hring, timeout_ms, &recv_req);
    if (rc == EAGAIN || rc == EINTR)
      continue;
    else if (rc == 0) {
      num_pkts++;
      num_bytes += recv_req.length;
      if (reinject >= 0) {
        rc = snf_inject_send(hinj, 10, recv_req.pkt_addr, recv_req.length);
        if (!rc)
          num_pkts_inject++;
      }
      if (netdev_reflect) {
        rc = snf_netdev_reflect(hnetdev, recv_req.pkt_addr, recv_req.length);
        if (!rc)
          num_pkts_netdev_reflect++;
      }
      if (!inspect_pkt)
        continue;
      if (verbose) {
        unsigned long long h_ts = host_nsecs();
        printf("pkt: %llu, len: %u, ts_hw: %llu ts_host: %llu ts_diff:%llu\n",
            (unsigned long long)num_pkts,
            recv_req.length, (unsigned long long) recv_req.timestamp,
            h_ts, h_ts - (unsigned long long) recv_req.timestamp);
      }
      if (snap_print > 0) {
          dump_data(recv_req.pkt_addr,
                   (recv_req.length > snap_print) ? snap_print : recv_req.length);
      }
    }
    else {
      fprintf(stderr, "error: snf_recv = %d (%s)\n",
                 rc, strerror(rc));
      break;
    }
  }

  if (hinj)
    snf_inject_close(hinj);

  rc = snf_ring_close(hring);
  if (rc) {
    errno = rc;
    perror("Can't close receive ring");
    return -1;
  }
  rc = snf_close(hsnf);
  if (rc) {
    errno = rc;
    perror("Can't close sniffer device");
    return -1;
  }

  return 0;
}

struct pkt_hdr {
  uint32_t length;
  uint32_t ofst;
};
typedef struct pkt_hdr pkt_hdr_t;
