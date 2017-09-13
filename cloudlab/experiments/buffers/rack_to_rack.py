#!/usr/bin/env python

import sys
sys.path.append("../../common/")

from common import RACKS, intializeExperiment, stopExperiment, waitOnNodes, \
    setQueueSize

iperf_server = 'iperf -s -D'
iperf_client = 'iperf -t1 -c %s'
ping = 'sudo ping -i.05 -w1 %s'

BUFFER_SIZES = [500]

initializeExperiment()

for host in RACKS[2]:
    runOnNode(host, iperf_server, 0, 0)
waitOnNodes()

for buffer_size in BUFFER_SIZES:
    setQueueSize(buffer_size)

    total = HOSTS_PER_RACK * 2;
    current = 0
    servers = RACKS[2].keys()
    for host in RACKS[1]:
        runOnNode(host, iperf_client % servers[current], current+1, total)
        current += 1

    for host in RACKS[1]:
        runOnNode(host, ping % servers[current], current+1, total)
        current += 1
    waitOnNodes()

stopExperiment()
