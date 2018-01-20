#!/usr/bin/python

import os
for i in range(1, 9):
    cmd = "ssh mukerjee@host%d \"cd /sdrt; git pull\""%(i)
    print cmd
    os.system(cmd)
