#!/usr/bin/env python

import sys
import dpkt

from collections import defaultdict


def get_ecn_hex(pkt):
    return ''.join("%02x" % x for x in bytearray(pkt[15]))


data = defaultdict(list)


pcap = list(dpkt.pcap.Reader(open(sys.argv[1], 'r')))
start_time = pcap[0][0]

current = get_ecn_hex(pcap[0][1])
start = 0
end = -1
total_payload = 0
num_packets = 0
for ts, pkt in pcap:
    ts = ts - start_time
    ecn = get_ecn_hex(pkt)
    if ecn != current:
        time = float(end - start)
        bw = total_payload / time if total_payload else 0
        # print current, start, num_packets, total_payload, time, bw
        data[current].append((time, bw))
        current = ecn
        start = end  # ts
        total_payload = 0
        num_packets = 0
    total_payload += len(pkt)
    end = ts
    num_packets += 1

for k, v in data.items():
    avg_time = zip(*v)[0]
    avg_time = sum(avg_time) / float(len(avg_time)) * 1000000
    avg_bw = zip(*v)[1]
    avg_bw = sum(avg_bw) / float(len(avg_bw)) / 1000000000
    print k, avg_time, avg_bw
