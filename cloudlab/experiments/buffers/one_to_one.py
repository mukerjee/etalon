#!/usr/bin/env python

import sys
import buffer_common
sys.path.append("../../common/")

from common import initializeExperiment, finishExperiment, flowgrind, \
    rack_ping, waitOnWork
from click_common import setConfig

initializeExperiment()

for config in buffer_common.CONFIGS:
    print '--- running test type %s...' % config['type']
    print '--- setting switch buffer size to %d...' % config['buffer_size']
    setConfig(config)
    print '--- done...'

    rack_ping(1, 2)

    settings = {'flows': []}
    settings['flows'].append({'src': 'r1', 'dst': 'r2'})
    flowgrind(settings)
    waitOnWork()

finishExperiment()
