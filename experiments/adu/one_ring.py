#!/usr/bin/env PYTHONPATH=../ python

import adu_common

from common import initializeExperiment, finishExperiment, flowgrind
from click_common import setConfig

adu_common.CONFIGS.append({'packet_log': False,
                           'type': 'fixed',
                           'traffic_source': 'ADU',
                           'fixed_schedule': '2 39600 7/0/1/2/3/4/5/6 400 -1/-1/-1/-1/-1/-1/-1/-1'})

adu_common.CONFIGS.append({'packet_log': False,
                           'type': 'fixed',
                           'traffic_source': 'ADU',
                           'buffer_size': 128,
                           'fixed_schedule': '2 39600 7/0/1/2/3/4/5/6 400 -1/-1/-1/-1/-1/-1/-1/-1'})

for config in adu_common.CONFIGS:
    initializeExperiment(adu=(config['traffic_source'] == 'ADU'))

    print '--- running test type %s...' % config
    setConfig(config)
    print '--- done...'

    settings = {'big_and_small': True}
    flowgrind(settings)

finishExperiment()
