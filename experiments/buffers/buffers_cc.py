#!/usr/bin/env python

import buffer_common

import sys
sys.path.insert(0, '/etalon/experiments')
from common import initializeExperiment, finishExperiment, flowgrind
from click_common import setConfig

CCS = ['reno', 'retcp']

initializeExperiment('flowgrindd')

for cc in CCS:
    for config in buffer_common.CONFIGS:
        if 'cc' in config:
            # Skip the reTCP experiments since they will be run anyway when
            # cc == 'retcp'.
            continue
        config['cc'] = cc

        print '--- running test type %s...' % config['type']
        print '--- using CC mode %s...' % config['cc']
        print '--- setting switch buffer size to %d...' % config['buffer_size']
        setConfig(config)
        print '--- done...'

        flowgrind(settings={"flows": [{'src': 'r1', 'dst': 'r2'}]})

finishExperiment()
