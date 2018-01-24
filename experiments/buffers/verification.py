#!/usr/bin/env python

import sys
import buffer_common
sys.path.append("../")

from common import initializeExperiment, finishExperiment, flowgrind
from click_common import setConfig

initializeExperiment()

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
    settings['flows'].append({'src': 'r2', 'dst': 'r1'})
    settings['flows'].append({'src': 'r3', 'dst': 'r4'})
    settings['flows'].append({'src': 'r4', 'dst': 'r3'})
    settings['flows'].append({'src': 'r5', 'dst': 'r6'})
    settings['flows'].append({'src': 'r6', 'dst': 'r5'})
    settings['flows'].append({'src': 'r7', 'dst': 'r8'})
    settings['flows'].append({'src': 'r8', 'dst': 'r7'})
    flowgrind(settings)

finishExperiment()