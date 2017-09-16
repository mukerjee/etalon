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
    rackToRackIperf3(1, 3)
    rackToRackIperf3(1, 4)
    rackToRackIperf3(1, 5)
    rackToRackIperf3(1, 6)
    rackToRackIperf3(1, 7)
    rackToRackIperf3(1, 8)

    rackToRackPing(1, 2)
    rackToRackPing(1, 3)
    rackToRackPing(1, 4)
    rackToRackPing(1, 5)
    rackToRackPing(1, 6)
    rackToRackPing(1, 7)
    rackToRackPing(1, 8)

    waitOnNodes()

stopExperiment()
