#!/usr/bin/env python

import sys
import buffer_common
sys.path.append("../../common/")

from common import initializeExperiment, finishExperiment, flowgrind, \
    rack_sockperf, waitOnWork
from click_common import setConfig

initializeExperiment()

for config in buffer_common.CONFIGS:
    print '--- running test type %s...' % config['type']
    print '--- setting switch buffer size to %d...' % config['buffer_size']
    setConfig(config)
    print '--- done...'

    settings = {'flows': []}
    settings['flows'].append({'src': 'r1', 'dst': 'r3'})
    flowgrind(settings)

finishExperiment()
