#!/usr/bin/env python

import sys
sys.path.append("../../common/")
import time
import os

from common import RACKS, HANDLE_TO_IP, initializeExperiment, waitOnNodes, setQueueSize, runOnNode
import common

TIMESTAMP = int(time.time())
SCRIPT = '.'.join(os.path.basename(__file__).split('.')[:-1])

iperf_client = 'iperf3 -i0.1 -t1 -c %s'
ping = 'sudo ping -i0.05 -w1 %s'

BUFFER_SIZES = [10, 100, 1000]

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
        runOnNode(host.hostname, iperf_client % (servers[current].hostname),
                  current+1, total, fn=out_fn, preload=False)
        current += 1

    for host in RACKS[1]:
        out_fn = '%s-%s-%s-%s-%s.txt' % (TIMESTAMP, SCRIPT, buffer_size, 
                                         host.hostname, 'ping')
        runOnNode(host.hostname, ping % (servers[current % HOSTS_PER_RACK].hostname), 
                  current+1, total, fn=out_fn)
        current += 1
    waitOnNodes()
