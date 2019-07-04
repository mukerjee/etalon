#!/usr/bin/env python
#
# A simple configuration that sets up the testbed, launches flowgrindd on the
# hosts, runs the Solstice scheduler, and exits, allowing the user to play
# around manually.

import sys
sys.path.insert(0, '/etalon/experiments')
from common import initializeExperiment
from click_common import setConfig

initializeExperiment('flowgrindd')

config = {
    'type': 'normal',
    'buffer_size': 128,
    'divert_acks': True
}

print '--- running test type %s...' % config['type']
print '--- setting switch buffer size to %d...' % config['buffer_size']
setConfig(config)
print '--- done...'
