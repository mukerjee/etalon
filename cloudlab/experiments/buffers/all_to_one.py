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

    rackToRackIperf3(2, 1)
    rackToRackIperf3(3, 1)
    rackToRackIperf3(4, 1)
    rackToRackIperf3(5, 1)
    rackToRackIperf3(6, 1)
    rackToRackIperf3(7, 1)
    rackToRackIperf3(8, 1)

    rackToRackPing(2, 1)
    rackToRackPing(3, 1)
    rackToRackPing(4, 1)
    rackToRackPing(5, 1)
    rackToRackPing(6, 1)
    rackToRackPing(7, 1)
    rackToRackPing(8, 1)

    waitOnNodes()

stopExperiment()
