#!/usr/bin/env python

import buffer_common

import sys
sys.path.insert(0, '/etalon/experiments')
from common import initializeExperiment, finishExperiment, flowgrind
from click_common import setConfig

# All available CC modules. Found by:
#     sudo sysctl net.ipv4.tcp_available_congestion_control
CCS = ['reno', 'cubic', 'retcp', 'dctcp', 'bbr', 'bic', 'cdg', 'highspeed',
       'htcp', 'hybla', 'illinois', 'lp', 'nv', 'scalable', 'vegas', 'veno',
       'westwood', 'yeah']

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
