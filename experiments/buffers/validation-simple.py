#!/usr/bin/env python
#
# A simple configuration that sets of the testbed, runs the Solstice scheduler,
# and exits, allowing the user to play around manually.

import sys
sys.path.insert(0, '/etalon/experiments')
from common import initializeExperiment
from click_common import setConfig

initializeExperiment('flowgrindd')

configs = [{'type': 'normal'}]

for c in configs:
    c['buffer_size'] = 128
    c['divert_acks'] = True

for config in configs:
    print '--- running test type %s...' % config['type']
    print '--- setting switch buffer size to %d...' % config['buffer_size']
    setConfig(config)
    print '--- done...'
