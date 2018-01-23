#!/usr/bin/python

import os
BASE="10.2"

for i in range(1, 9):
    for j in range(1, 17):
        addr = '%s.%d.%d' % (BASE, i, j)
        cmd = "scp hosts root@%s:/etc/"%(addr)
        os.system(cmd)
