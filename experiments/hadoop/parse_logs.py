#!/usr/bin/env python

import sys
import glob
import numpy as np

data = []
durations = []

fn = '/tmp/' + sys.argv[1] + '-logs/*-logs/hadoop*-datanode*.log'
print fn
logs = glob.glob(fn)

for log in logs:
    for line in open(log):
        if 'clienttrace' in line and 'HDFS_WRITE' in line:
            bytes = int(line.split('bytes: ')[1].split()[0][:-1])
            if bytes > 1024 * 1024 * 99:
                duration = float(line.split("duration: ")[1].split()[0])*1e-9
                data.append((bytes / 1024. / 1024., duration))

durations = sorted(zip(*data)[1])
print durations
print np.percentile(durations, 50), np.percentile(durations, 99)
