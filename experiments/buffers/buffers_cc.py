#!/usr/bin/env python

import copy
import sys
sys.path.insert(0, '/etalon/experiments')

import buffer_common
from click_common import setConfig
from common import initializeExperiment, finishExperiment, flowgrind

# All available CC modules. Found by:
#     sudo sysctl net.ipv4.tcp_available_congestion_control
CCS = ['reno', 'cubic', 'retcp', 'dctcp', 'bbr', 'bic', 'cdg', 'highspeed',
       'htcp', 'hybla', 'illinois', 'lp', 'nv', 'scalable', 'vegas', 'veno',
       'westwood', 'yeah']

initializeExperiment('flowgrindd')

# Total number of experiments.
TOT = len(CCS) * len(buffer_common.CONFIGS)
CNT = 0
for cc in CCS:
    for config in buffer_common.CONFIGS:
        CNT += 1

        # Make a copy so that the if-statement below does not trigger after the
        # first CC mode.
        config_c = copy.deepcopy(config)

        if 'cc' in config_c:
            # Skip the reTCP experiments because they will be run anyway when
            # cc == 'retcp'.
            continue

        config_c['cc'] = cc
        print '--- running test type %s...' % config_c['type']
        print '--- using CC mode %s...' % config_c['cc']
        print '--- setting switch buffer size to %d...' % config_c['buffer_size']
        setConfig(config_c)
        print '--- done...'
        print '--- experiment %d of %d' % (CNT, TOT)
        flowgrind(settings={'flows': [{'src': 'r1', 'dst': 'r2'}]})

finishExperiment()
