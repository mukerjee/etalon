#!/usr/bin/env python

import sys
sys.path.insert(0, '/etalon/experiments')
from common import initializeExperiment, finishExperiment, flowgrind
from click_common import setConfig

initializeExperiment('flowgrindd')

configs = [{'type': 'strobe'}]
for c in configs:
    c['buffer_size'] = 16
    c['divert_acks'] = True

for config in configs:
    print '--- running test type %s...' % config['type']
    print '--- setting switch buffer size to %d...' % config['buffer_size']
    setConfig(config)
    print '--- done...'

    settings = {'flows': []}
    # A ring: 1->2, 2->3, 3->1. Mimics DEFAULT_CIRCUIT_CONFIG.
    settings['flows'].append({'src': 'r1', 'dst': 'r2'})
    settings['flows'].append({'src': 'r2', 'dst': 'r3'})
    settings['flows'].append({'src': 'r3', 'dst': 'r1'})
    flowgrind(settings)

finishExperiment()
