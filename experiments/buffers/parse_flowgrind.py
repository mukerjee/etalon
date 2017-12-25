#!/usr/bin/env python

import sys
import glob
import os
import tarfile

from collections import defaultdict

flows = defaultdict(list)


def parse_flowgrind(fn):
    for l in open(fn):
        if 'seed' in l:
            l = l[1:].strip().split(',')
            id = int(l[0].split()[0])
            rack = int(l[0].split()[2].split('/')[0].split('.')[2])
            flows[id].append(rack)
            if l[0].split()[1][0] == 'S':
                time = float(l[7].split('=')[1].strip().split('/')[0][:-1])
                tp = float(l[8].split('=')[1].strip().split('/')[0]) / 1000.0
            else:
                flows[id].append(time)
                flows[id].append(tp)

    print
    print fn
    for f in flows:
        print f, flows[f]

    pairs = list(set([(f[0], f[1]) for f in flows.values()]))
    print pairs

    tps = defaultdict(list)
    for f in flows.values():
        sd = (f[0], f[1])
        tps[sd].append(f[3])

    print tps
    for sd in tps:
        print sd, sum(tps[sd])
            

if __name__ == "__main__":
    fns = sys.argv[1]
    if 'tar.gz' in fns:
        dir = '/tmp/' + fns.split('.tar.gz')[0].split('/')[-1]
        if not os.path.isdir(dir):
            os.mkdir(dir)
        t = tarfile.open(fns)
        t.extractall(dir)
        fns = dir + '/*'
    # if 'tmp' not in fns:
    #     fns += '/tmp/*'
    for fn in glob.glob(fns):
        parse_flowgrind(fn)
