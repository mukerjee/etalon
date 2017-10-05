#!/usr/bin/env python

import sys
import struct
import dpkt


def get_length(p):
    if len(p) == 34:
        return struct.unpack('!H', bytearray(p[16:18]))[0] + 14
    else:
        return None

pcap = dpkt.pcap.Reader(open(sys.argv[1], 'r'))
start_time = -1

total_payload = 0
for ts, p in pcap:
    if start_time == -1:
        start_time = ts
    l = get_length(p)
    if l is None:
        break
    total_payload += l
    end_time = ts

total_time = end_time - start_time

print total_time, (total_payload*8) / float(total_time) / 1000000000
