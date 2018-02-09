#!/usr/bin/env python

import sys
sys.path.insert(0, '..')
from common import initializeExperiment, finishExperiment, flowgrind
from click_common import setConfig

CONFIGS = [
    {'buffer_size':   4},
    {'buffer_size':   8},
    {'buffer_size':  16},
    {'buffer_size':  32},
    {'buffer_size':  64},
    {'buffer_size': 128},
    {'buffer_size':  16, 'in_advance': 20000, 'queue_resize': True},
]

#               0.5    1    1.5    2     3     4     6      8      12
link_delays = [1e-4, 2e-4, 3e-4, 4e-4, 6e-4, 8e-4, 12e-4, 16e-4, 24e-4]
sched = ['strobe']

initializeExperiment()

for t in sched:
    for d in link_delays:
        for config in CONFIGS:
            config['circuit_link_delay'] = d
            config['type'] = t
            if t == 'short_reconfig' and 'in_advance' in config:
                config['in_advance'] = 10000
            print '--- running test type %s...' % config['type']
            print '--- setting switch buffer size to %d...' % \
                config['buffer_size']
            setConfig(config)
            print '--- done...'

            settings = {'flows': []}
            settings['flows'].append({'src': 'r1', 'dst': 'r2'})
            flowgrind(settings)

finishExperiment()
