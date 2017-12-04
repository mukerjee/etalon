#!/usr/bin/env python

import sys
import glob
import numpy as np

from collections import defaultdict

fns = sys.argv[1]
if 'tmp' not in fns:
    fns += '/tmp/*'

percentiles = [25, 50, 75, 99, 99.9, 99.99, 99.999, 100]

for fn in glob.glob(fns):
    latencies = []
    throughputs = defaultdict(int)
    flow_start = {}
    flow_end = {}
    for l in open(fn):
        ts, d = l.split(':')
        d = d.split(',')
        bytes = int(d[0].split('(')[1].split(' ')[0])
        sender = int(d[0].split('->')[0])
        recv = int(d[0].split('->')[1].split('(')[0])
        latency = float(d[-2].split(' ')[-1].split('us')[0])
        sr = (sender, recv)
        if bytes < 100:
            continue
        if sr not in flow_start:
            flow_start[sr] = float(ts) / 20.0  # TDF
        throughputs[sr] += bytes
        flow_end[sr] = float(ts) / 20.0  # TDF
        if bytes > 1000:  # 9014:
            latencies.append(latency)
    lat = zip(percentiles, map(lambda x: np.percentile(latencies, x),
                               percentiles))
    tp = {}
    for sr in flow_start:
        tp[sr] = throughputs[sr] / (flow_end[sr] - flow_start[sr])
        tp[sr] *= 8  # bytes to bits
        tp[sr] /= 1000000000  # bits to gbits
    print
    print fn
    print tp
    print lat
