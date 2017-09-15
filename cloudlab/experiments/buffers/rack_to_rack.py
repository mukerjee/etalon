#!/usr/bin/env python

import sys
sys.path.append("../../common/")
import os
import time
import tarfile
import glob

from common import initializeExperiment, rackToRackIperf3, rackToRackPing, \
    waitOnNodes, setQueueSize

TIMESTAMP = int(time.time())
SCRIPT = '.'.join(os.path.basename(__file__).split('.')[:-1])

BUFFER_SIZES = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]

initializeExperiment()

experiments = []
for buffer_size in BUFFER_SIZES:
    print '--- setting switch buffer size to %d...' % buffer_size
    setQueueSize(buffer_size)
    print '--- done...'

    out_fn = '%s-%s-%s' % (TIMESTAMP, SCRIPT, buffer_size)
    rackToRackIperf3(1, 2, out_fn)
    # rackToRackIperf3(2, 1, out_fn)

    rackToRackIperf3(3, 4, out_fn)
    # rackToRackIperf3(4, 3, out_fn)

    rackToRackIperf3(5, 6, out_fn)
    # rackToRackIperf3(6, 5, out_fn)

    rackToRackIperf3(7, 8, out_fn)
    # rackToRackIperf3(8, 7, out_fn)

    rackToRackPing(1, 2, out_fn)
    # rackToRackPing(2, 1, out_fn)

    rackToRackPing(3, 4, out_fn)
    # rackToRackPing(4, 3, out_fn)

    rackToRackPing(5, 6, out_fn)
    # rackToRackPing(6, 5, out_fn)

    rackToRackPing(7, 8, out_fn)
    # rackToRackPing(8, 7, out_fn)

    waitOnNodes()
    experiments.append(out_fn)


tar = tarfile.open("%s-%s.tar.gz" % (TIMESTAMP, SCRIPT), "w:gz")
for e in experiments:
    for fn in glob.glob(e+"*.txt"):
        tar.add(fn)
tar.close()

for fn in glob.glob('*.txt'):
    os.remove(fn)
