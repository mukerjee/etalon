#!/usr/bin/env python

import sys
import struct
import dpkt

import numpy as np

from collections import defaultdict


def get_ecn_hex(pkt):
    if len(pkt) == 34:
        return ''.join("%02x" % x for x in bytearray(pkt[15]))
    else:
        return None


def get_length(p):
    if len(p) == 34:
        return struct.unpack('!H', bytearray(p[16:18]))[0] + 14
    else:
        return None


data = defaultdict(list)


pcap = dpkt.pcap.Reader(open(sys.argv[1], 'r'))
start_time = -1

current = -1
start = 0
end = -1
total_payload = 0
num_packets = 0
try:
    for ts, pkt in pcap:
        if start_time == -1:
            start_time = ts
        if current == -1:
            current = get_ecn_hex(pkt)
        ts = ts - start_time
        ecn = get_ecn_hex(pkt)
        if ecn is None:
            break
        if ecn != current:
            time = float(end - start)
            bw = total_payload / time if time else 0
            # print current, start, num_packets, total_payload, time, bw
            data[current].append((time, bw))
            current = ecn
            start = end  # ts
            total_payload = 0
            num_packets = 0
        total_payload += get_length(pkt)
        end = ts
        num_packets += 1
except:
    pass

for k, v in data.items():
    avg_time = zip(*v)[0]
    med_time = np.median(avg_time) * 1000000
    avg_time = sum(avg_time) / float(len(avg_time)) * 1000000
    avg_bw = zip(*v)[1]
    med_bw = np.median(avg_bw)*8 / 1000000000.0
    avg_bw = (sum(avg_bw)*8) / float(len(avg_bw)) / 1000000000
    print k, med_time, avg_time, '\t', med_bw, avg_bw
    # print k, avg_time, avg_bw
