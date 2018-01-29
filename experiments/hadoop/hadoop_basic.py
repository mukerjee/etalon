#!/usr/bin/env python

import sys
sys.path.append("../")

import os

from common import initializeExperiment, finishExperiment, dfsioe
from click_common import setConfig

CONFIGS = [
    {'type': 'normal', 'traffic_source': 'QUEUE'},
    {'type': 'resize', 'traffic_source': 'QUEUE', 'in_advance': 20000},
    {'type': 'resize', 'traffic_source': 'QUEUE', 'in_advance': 20000, 'cc': 'ocs'},
    {'type': 'normal', 'traffic_source': 'ADU'},
    {'type': 'resize', 'traffic_source': 'ADU', 'in_advance': 20000},
    {'type': 'resize', 'traffic_source': 'ADU', 'in_advance': 20000, 'cc': 'ocs'},
]

for c in CONFIGS:
    c['packet_log'] = False
    if c['type'] == 'resize':
        c['type'] = 'normal'
        c['queue_resize'] = True
    else:
        c['queue_resize'] = False
    if 'cc' not in c:
        c['cc'] = 'reno'

for h in ['Hadoop', 'Hadoop-SDRT']:
    for c in CONFIGS:
        c['hadoop'] = h
        print '--- running %s, %s, %s, %s' % (h, c['traffic_source'], c['queue_resize'], c['cc'])
        initializeExperiment(adu=(c['traffic_source']=='ADU'), hadoop=c['hadoop'])
        setConfig(c)
        print '--- done initializing...'
        dfsioe('h21', h)

finishExperiment()
