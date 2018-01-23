#!/usr/bin/python

import os
BASE="10.2"

for i in range(1, 9):
    for j in range(1, 17):
        addr = '%s.%d.%d' % (BASE, i, j)
        cmd = "ssh root@%s pkill -9 java"%(addr)
        os.system(cmd)
        cmd = "ssh root@%s rm -rf /tmp"%(addr)
        os.system(cmd)
        cmd = "ssh root@%s rm -rf /usr/local/hadoop/logs/*"%(addr)
        os.system(cmd)
