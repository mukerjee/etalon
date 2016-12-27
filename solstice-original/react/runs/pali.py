#!/usr/bin/env python

import os

def c(cmd):
    print cmd
    return os.system(cmd)

for i in range(1, 8):
    for j in range(8-i):
        c('mkdir pali%d%d' % (i, j))
        c('ln -s ../make.mk pali%d%d/makefile' % (i, j))
        f = open('pali%d%d/react.conf' % (i, j), 'w')
        print >> f, '''{
            "Demand": "cali",
            "Sched": {
                "Diag": true,
                "PureCircuit": true,
                "FullBandw": true,
                "CleanOnIdle": true
            },
            "DemCali": {
                "Nbig": %d,
                "Nsmall": %d
            }
        }
        ''' % (i, j)
        f.close()
