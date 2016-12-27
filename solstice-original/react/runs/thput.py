#!/usr/bin/env python

import os

def c(cmd):
    print cmd
    return os.system(cmd)

c('rm thput/thput')
for i in range(1, 8):
    for j in range(8-i):
        c('echo .cali%d%d >> thput/thput' % (i, j))
        c('../bin/thput -rep=cali%d%d/sim.rep >> thput/thput' % (i, j))
        c('../bin/thput -rep=cali%d%d/run.rep >> thput/thput' % (i, j))
        c('echo .pali%d%d >> thput/thput' % (i, j))
        c('../bin/thput -rep=pali%d%d/sim.rep >> thput/thput' % (i, j))
        c('../bin/thput -rep=pali%d%d/run.rep >> thput/thput' % (i, j))
