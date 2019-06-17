#!/usr/bin/env python

import sys
sys.path.insert(0, '/etalon/experiments')
from common import initializeExperiment, finishExperiment, flowgrind
from click_common import setConfig

initializeExperiment('flowgrindd')

configs = [{'type': 'no_circuit'}, {'type': 'circuit'}]

for c in configs:
    c['buffer_size'] = 128
    c['divert_acks'] = True

for config in configs:
    print '--- running test type %s...' % config['type']
    print '--- setting switch buffer size to %d...' % config['buffer_size']
    setConfig(config)
    print '--- done...'

    settings = {'flows': []}
    settings['flows'].append({'src': 'r1', 'dst': 'r2'})
    settings['flows'].append({'src': 'r2', 'dst': 'r1')}
    settings['flows'].append({'src': 'r2', 'dst': 'r3'})
    settings['flows'].append({'src': 'r3', 'dst': 'r2'})
    flowgrind(settings)

finishExperiment()
