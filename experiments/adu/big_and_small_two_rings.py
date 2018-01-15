#!/usr/bin/env python

import sys
import adu_common
sys.path.append("../")

from common import initializeExperiment, finishExperiment, flowgrind
from click_common import setConfig

for config in adu_common.CONFIGS:
    initializeExperiment(adu=(config['traffic_source']=='ADU'))

    print '--- running test type %s...' % config
    setConfig(config)
    print '--- done...'

    settings = {'big_and_small_two_rings': True}
    flowgrind(settings)

finishExperiment()
