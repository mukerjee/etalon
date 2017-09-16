#!/usr/bin/env python

import os
import sys
import buffer_common
sys.path.append("../../common/")

from common import initializeExperiment, rackToRackIperf3, rackToRackPing, \
    waitOnNodes, setConfig, stopExperiment

initializeExperiment()

for config in buffer_common.CONFIGS:
    print '--- running test type %s...' % config['type']
    print '--- setting switch buffer size to %d...' % config['buffer_size']
    setConfig(config)
    print '--- done...'

    rackToRackIperf3(1, 2)
    rackToRackIperf3(2, 3)
    rackToRackIperf3(3, 4)
    rackToRackIperf3(4, 5)
    rackToRackIperf3(5, 6)
    rackToRackIperf3(6, 7)
    rackToRackIperf3(7, 8)
    rackToRackIperf3(8, 1)

    rackToRackPing(1, 2)
    rackToRackPing(2, 3)
    rackToRackPing(3, 4)
    rackToRackPing(4, 5)
    rackToRackPing(5, 6)
    rackToRackPing(6, 7)
    rackToRackPing(7, 8)
    rackToRackPing(8, 1)

    waitOnNodes()

stopExperiment()
