#!/usr/bin/env python

from dotmap import DotMap

import sys
sys.path.insert(0, '../../')
sys.path.insert(0, '/Users/mukerjee/Dropbox/Research/simpleplotlib/')

import shelve
import os
import glob
import copy
import numpy as np

from collections import defaultdict
from struct import unpack
from get_throughput_and_latency import msg_from_file
from simpleplotlib import plot

bin_size = 1

def plot_tput(fns, packet):
    if not os.path.isdir(sys.argv[1]):
        print 'first arg must be dir'
        sys.exit(-1)
    db = shelve.open(sys.argv[1] + '/tp_shelve.db')
    key = 'packet' if packet else 'circuit'
    if key in db:
        tp_out = db[key]
    else:
        tp_out = {}
        for fn in fns:
            id_to_sr = {}
            for l in open(fn.split('.txt')[0] + '.config.txt'):
                l = l.split('-F')[1:]
                for x in l:
                    id = int(x.strip().split()[0])
                    s = int(x.strip().split('-Hs=10.1.')[1].split('.')[0])
                    r = int(x.strip().split(',d=10.1.')[1].split('.')[0])
                    id_to_sr[id] = (s, r)

            out_data = defaultdict(lambda: defaultdict(int))
            circuit = False if 'no_circuit' in fn else True
            for line in open(fn):
                if line[0] == 'S' and (line[1] == ' ' or line[1] == '1'):
                    line = line[1:].strip()
                    id = int(line.split()[0])
                    ts_start = float(line.split()[1])
                    ts_end = float(line.split()[2])
                    curr_tp = float(line.split()[3]) * 1e6
                    curr_bytes = (curr_tp / 8.0) * (ts_end - ts_start)
                    out_data[id_to_sr[id]][ts_start] += curr_bytes
            for sr in out_data:
                out_data[sr] = sorted(out_data[sr].items())

            fn_ts = fn.split('/')[-1].split('-verification')[0]
            if circuit:
                packet_log_fn = glob.glob(sys.argv[1] +
                                          '/tmp/%s-verification-circuit-*-click.txt'
                                          % fn_ts)
            else:
                packet_log_fn = glob.glob(sys.argv[1] +
                                          '/tmp/%s-verification-no_circuit-*-click.txt'
                                          % fn_ts)
            if packet_log_fn:
                out_data = defaultdict(list)
                first_ts = -1
                for msg in msg_from_file(packet_log_fn[0]):
                    (t, ts, lat, src, dst, data) = unpack('i32siii64s', msg)
                    if t != 0:
                        continue
                    bytes = unpack('!H', data[2:4])[0]
                    sender = ord(data[14])
                    recv = ord(data[18])
                    # sender = ord(data[15])
                    # recv = ord(data[19])
                    sr = (sender, recv)
                    for i in xrange(len(ts)):
                        if ord(ts[i]) == 0:
                            break
                    ts = (float(ts[:i]) / 20.0) * 1000  # TDF
                    if first_ts == -1:
                        first_ts = ts
                    out_data[sr].append((ts - first_ts, bytes))

            print 'done parsing ' + fn
            tp = defaultdict(list)
            for sr in out_data:
                if packet_log_fn:
                    curr = 0
                    for i in xrange(0, 2000, bin_size):
                        curr_tp = 0
                        for j in xrange(curr, len(out_data[sr])):
                            ts, bytes = out_data[sr][j]
                            if ts >= i and ts < i+bin_size:
                                curr_tp += bytes
                            else:
                                curr = j
                                break
                        tp[sr].append(curr_tp * (1000.0 / bin_size) * 8.0 / 1e9)
                else:
                    for i in xrange(0, 2000, bin_size):
                        curr = [b for ts, b in out_data[sr]
                                    if ts >= i/1000.0 and ts < (i+bin_size) / 1000.0]
                        curr = sum(curr)
                        tp[sr].append((curr*8.0 / (bin_size / 1000.0) / 1e9))
            tp_out[fn] = copy.deepcopy(tp)
    db[key] = tp_out
    tp = defaultdict(list)
    for sr in tp_out[fns[0]]:
        for i in xrange(0, 2000, bin_size):
            tp[sr].append(np.average([q[sr][i] for q in tp_out.values()]))
    print [len(tp[k]) for k in tp.keys()]
    x = [xrange(0, 2000, bin_size) for sr in tp.keys()]
    y = tp.values()

    means = []
    stds = []
    for data in y:
        for i, d in enumerate(data):
            if round(d) > 0:
                break
        means.append(np.mean(data[i:]))
        stds.append(np.std(data[i:]))
    print means
    print stds
    print np.mean(means)
    print np.mean(stds)

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = [str(q) for q in tp.keys()]
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, marker='o',
                                     markersize='1', linewidth=0)
                              for i in range(len(x))]

    if not packet:
        options.output_fn = 'graphs/verification_circuit.pdf'
        options.y.limits = [0, 90]
    else:
        options.output_fn = 'graphs/verification_packet.pdf'
        options.y.limits = [0, 12]
    options.x.label.xlabel = 'Time (ms)'
    options.y.label.ylabel = 'Rack throughput (Gbps)'
    plot(x, y, options)
    db.close()

    
if __name__ == '__main__':
    plot_tput(glob.glob(sys.argv[1] +
                        '/*-verification-no_circuit-*-flowgrind.txt'), True)
    plot_tput(glob.glob(sys.argv[1] +
                        '/*-verification-circuit-*-flowgrind.txt'), False)
