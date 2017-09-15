#!/usr/bin/env python

import sys
sys.path.append("../../common/")
import time
import os

from common import RACKS, HANDLE_TO_IP, initializeExperiment, waitOnNodes, setQueueSize, runOnNode, waitOnWork
import common

TIMESTAMP = int(time.time())
SCRIPT = '.'.join(os.path.basename(__file__).split('.')[:-1])

# iperf_client = 'iperf3 -p53%s -i0.1 -t1 -c %s'
# iperf_client = 'nuttcp -P53%s %s'
# netperf_client = 'netperf -P0 -H %s -l10 -- -o THROUGHPUT,P50_LATENCY,P90_LATENCY,P99_LATENCY'
# netperf_client = 'iperf -i0.5 -t1 -c %s'
# ping = 'sudo ping -i0.05 -w1 %s'

BUFFER_SIZES = [1000]

initializeExperiment()
HOSTS_PER_RACK = common.HOSTS_PER_RACK

for buffer_size in BUFFER_SIZES:
    print '--- setting switch buffer size to %d...' % buffer_size
    setQueueSize(buffer_size)
    print '--- done...'

    total = HOSTS_PER_RACK * 2
    current = 0
    servers = RACKS[2]
    for host in RACKS[1]:
        out_fn = '%s-%s-%s-%s-%s.txt' % (TIMESTAMP, SCRIPT, buffer_size, 
                                         host.hostname, 'iperf3')
        host.iperf3(servers[current], out_fn)
        # runOnNode(host, iperf_client % (host[1:], servers[current]), current+1, total, fn=out_fn)
        # runOnNode(HANDLE_TO_IP[host], netperf_client % (servers[current]), current+1, total, fn=out_fn)
        current += 1

    for host in RACKS[1]:
        out_fn = '%s-%s-%s-%s-%s.txt' % (TIMESTAMP, SCRIPT, buffer_size, 
                                         host.hostname, 'ping')
        host.ping(servers[current % HOSTS_PER_RACK], out_fn)
        # runOnNode(host, ping % servers[current], current+1, total, fn=out_fn)
        current += 1
    waitOnWork()
