#!/usr/bin/env python

import sys
sys.path.append("../")

from common import initializeExperiment, finishExperiment, flowgrind
from click_common import setConfig

initializeExperiment(hadoop=True)

#for config in buffer_common.CONFIGS:
#    print '--- running test type %s...' % config['type']
#    print '--- setting switch buffer size to %d...' % config['buffer_size']
#    setConfig(config)
#    print '--- done...'
#
#    settings = {'flows': []}
#    settings['flows'].append({'src': 'r1', 'dst': 'r2'})
#    flowgrind(settings)

#finishExperiment()
