#!/usr/bin/env python

import sys
import adu_common
sys.path.append("../")

from common import initializeExperiment, finishExperiment, flowgrind
from click_common import setConfig

for config in adu_common.CONFIGS:
    initializeExperiment(adu=(config['traffic_source']=='ADU'))

    print '--- running test type %s...' % config['type']
    print '--- setting switch buffer size to %d...' % config['buffer_size']
    setConfig(config)
    print '--- done...'

    settings = {'empirical': 'FB'}
    flowgrind(settings)

finishExperiment()
