#!/usr/bin/env python

import sys
sys.path.append("../")

import os

from common import initializeExperiment, finishExperiment, flowgrind
from click_common import setConfig

initializeExperiment(hadoop=True)
nn = 'h11.sdrt.cs.cmu.edu'
cmd = 'scp -o StrictHostKeyChecking=no ~/HiBench.tar.gz root@%s:/root/' % nn
os.system(cmd)
cmd = 'ssh -o StrictHostKeyChecking=no root@%s ' \
      '"tar xfvz HiBench.tar.gz &> /dev/null && rm HiBench.tar.gz"' % nn
dnull = os.system(cmd)

#for config in buffer_common.CONFIGS:
#    print '--- running test type %s...' % config['type']
#    print '--- setting switch buffer size to %d...' % config['buffer_size']
#    setConfig(config)
#    print '--- done...'
#
#    settings = {'flows': []}
#    settings['flows'].append({'src': 'r1', 'dst': 'r2'})
#    flowgrind(settings)

#finishExperiment()
