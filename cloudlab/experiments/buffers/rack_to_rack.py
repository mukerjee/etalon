#!/usr/bin/env python

import sys
sys.path.append("../../common/")
import time
import os

from common import RACKS, initializeExperiment, waitOnNodes, setQueueSize, runOnNode
import common

TIMESTAMP = int(time.time())
SCRIPT = '.'.join(os.path.basename(__file__).split('.')[:-1])

iperf_client = 'iperf3 -p53%s -i0.1 -t1 -c %s'
ping = 'sudo ping -i0.05 -w1 %s'

BUFFER_SIZES = [100]

initializeExperiment()
HOSTS_PER_RACK = common.HOSTS_PER_RACK

for buffer_size in BUFFER_SIZES:
    print '--- setting switch buffer size to %d...' % buffer_size
    setQueueSize(buffer_size)
    print '--- done...'

    total = HOSTS_PER_RACK * 2;
    current = 0
    servers = RACKS[2]
    for host in [RACKS[1][0]]:
        out_fn = '%s-%s-%s-%s.txt' % (TIMESTAMP, SCRIPT, host, 'iperf')
        runOnNode(host, iperf_client % (host[1:], servers[current]), current+1, total, fn=out_fn)
        current += 1

    for host in [RACKS[1][0]]:
        out_fn = '%s-%s-%s-%s.txt' % (TIMESTAMP, SCRIPT, host, 'ping')
        runOnNode(host, ping % servers[current], current+1, total, fn=out_fn)
        current += 1
    waitOnNodes()
