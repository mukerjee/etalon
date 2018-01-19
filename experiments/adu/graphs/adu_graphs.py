#!/usr/bin/env python

from dotmap import DotMap

import sys
sys.path.insert(0, '../../')
sys.path.insert(0, '/Users/mukerjee/Dropbox/Research/simpleplotlib/')

import copy
import glob
import numpy as np

from collections import defaultdict
from tabulate import tabulate
from get_throughput_and_latency import get_tput_and_dur
from simpleplotlib import plot

SMALL_FLOW_MAX_SIZE = 1000000


def graph(data, x_label, fn):
    graph_small_durs(data, x_label, fn)
    graph_big_durs(data, x_label, fn)
    graph_small_tp(data, x_label, fn)
    graph_big_tp(data, x_label, fn)


def graph_small_durs(d2, x_label, fn):
    data = copy.deepcopy(d2)
    for i in xrange(len(data)):
        data[i] = [[d[0], d[1] / 1000.0, d[2]]
                   for d in data[i] if d[2] <= SMALL_FLOW_MAX_SIZE]
    x = [sorted(zip(*data[0])[1]), sorted(zip(*data[1])[1]),
         sorted(zip(*data[2])[1])]
    y = [[float(i) / (len(x[0])-1) * 100 for i in xrange(len(x[0]))]]
    y.append([float(i) / (len(x[1])-1) * 100 for i in xrange(len(x[1]))])
    y.append([float(i) / (len(x[2])-1) * 100 for i in xrange(len(x[2]))])
    print
    print len(x[0])
    
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Today', 'ADU', 'Fixed']
    options.legend.options.fontsize = 12
    options.x.limits = [0, 100]
    options.output_fn = 'graphs/mice_fct_cdf.pdf'
    options.x.label.xlabel = 'Flow completion time (ms)'
    options.y.label.ylabel = 'CDF (%)'
    plot(x, y, options)


def graph_big_durs(d2, x_label, fn):
    data = copy.deepcopy(d2)
    for i in xrange(len(data)):
        data[i] = [[d[0], d[1] / 1000.0, d[2]]
                   for d in data[i] if d[2] > SMALL_FLOW_MAX_SIZE]
    x = [sorted(zip(*data[0])[1]), sorted(zip(*data[1])[1]),
         sorted(zip(*data[2])[1])]
    y = [[float(i) / (len(x[0])-1) * 100 for i in xrange(len(x[0]))]]
    y.append([float(i) / (len(x[1])-1) * 100 for i in xrange(len(x[1]))])
    y.append([float(i) / (len(x[2])-1) * 100 for i in xrange(len(x[2]))])
    print
    print len(x[0])

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Today', 'ADU', 'Fixed']
    options.legend.options.fontsize = 12
    options.output_fn = 'graphs/elephant_fct_cdf.pdf'
    options.x.label.xlabel = 'Flow completion time (ms)'
    options.y.label.ylabel = 'CDF (%)'
    plot(x, y, options)


def graph_small_tp(d2, x_label, fn):
    data = copy.deepcopy(d2)
    for i in xrange(len(data)):
        data[i] = [d for d in data[i] if d[2] <= SMALL_FLOW_MAX_SIZE]
    x = [sorted(zip(*data[0])[0]), sorted(zip(*data[1])[0]),
         sorted(zip(*data[2])[0])]
    y = [[float(i) / (len(x[0])-1) * 100 for i in xrange(len(x[0]))]]
    y.append([float(i) / (len(x[1])-1) * 100 for i in xrange(len(x[1]))])
    y.append([float(i) / (len(x[2])-1) * 100 for i in xrange(len(x[2]))])
    print
    print len(x[0])

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Today', 'ADU', 'Fixed']
    options.legend.options.fontsize = 12
    options.x.limits = [0, 10]
    options.output_fn = 'graphs/mice_tp_cdf.pdf'
    options.x.label.xlabel = 'Throughput (Gbps)'
    options.y.label.ylabel = 'CDF (%)'
    plot(x, y, options)


def graph_big_tp(d2, x_label, fn):
    data = copy.deepcopy(d2)
    for i in xrange(len(data)):
        data[i] = [d for d in data[i] if d[2] > SMALL_FLOW_MAX_SIZE]
    x = [sorted(zip(*data[0])[0]), sorted(zip(*data[1])[0]),
         sorted(zip(*data[2])[0])]
    y = [[float(i) / (len(x[0])-1) * 100 for i in xrange(len(x[0]))]]
    y.append([float(i) / (len(x[1])-1) * 100 for i in xrange(len(x[1]))])
    y.append([float(i) / (len(x[2])-1) * 100 for i in xrange(len(x[2]))])
    print
    print len(x[0])
    
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Today', 'ADU', 'Fixed']
    options.legend.options.fontsize = 12
    options.x.limits = [0, 10]
    options.output_fn = 'graphs/elephant_tp_cdf.pdf'
    options.x.label.xlabel = 'Throughput (Gbps)'
    options.y.label.ylabel = 'CDF (%)'
    plot(x, y, options)


def bytes_graph(d2):
    data = copy.deepcopy(d2)
    x = np.array([[0, 1], [0, 1], [0, 1]])
    y = [[sum(data['QUEUE'][0]), sum(data['QUEUE'][1])],
         [sum(data['ADU'][0]), sum(data['ADU'][1])],
         [sum(data['FIXED'][0]), sum(data['FIXED'][1])]]
    for i in xrange(len(y)):
        y[i] = [p / 1024.0 / 1024.0 / 1024.0 for p in y[i]]

    print x, y

    options = DotMap()
    options.plot_type = 'BAR'
    options.legend.options.labels = ['Today', 'ADU', 'Fixed']
    options.legend.options.fontsize = 12
    options.output_fn = 'graphs/utilization.pdf'
    options.x.label.xlabel = ''
    options.y.label.ylabel = 'Bytes sent (GB)'
    plot(x, y, options)

if __name__ == '__main__':
    all_file_data = defaultdict(list)
    queue_data = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-normal-*-'
                        'QUEUE-False-*-reno-flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        queue_data = zip(tputs, durs, bytes)

    all_file_data['big_tp'].append([d[0] for d in queue_data
                                    if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['big_dur'].append([d[1] for d in queue_data
                                     if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['small_tp'].append([d[0] for d in queue_data
                                      if d[2] <= SMALL_FLOW_MAX_SIZE])
    all_file_data['small_dur'].append([d[1] for d in queue_data
                                       if d[2] <= SMALL_FLOW_MAX_SIZE])

    resize_data = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-normal-*-'
                        'QUEUE-True-*-reno-flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        resize_data = zip(tputs, durs, bytes)

    all_file_data['big_tp'].append([d[0] for d in resize_data
                                    if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['big_dur'].append([d[1] for d in resize_data
                                     if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['small_tp'].append([d[0] for d in resize_data
                                      if d[2] <= SMALL_FLOW_MAX_SIZE])
    all_file_data['small_dur'].append([d[1] for d in resize_data
                                       if d[2] <= SMALL_FLOW_MAX_SIZE])

    ocs_data = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-normal-*-'
                        'QUEUE-True-*-ocs-flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        ocs_data = zip(tputs, durs, bytes)

    all_file_data['big_tp'].append([d[0] for d in ocs_data
                                    if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['big_dur'].append([d[1] for d in ocs_data
                                     if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['small_tp'].append([d[0] for d in ocs_data
                                      if d[2] <= SMALL_FLOW_MAX_SIZE])
    all_file_data['small_dur'].append([d[1] for d in ocs_data
                                       if d[2] <= SMALL_FLOW_MAX_SIZE])

    adu_data = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-normal-*-'
                        'ADU-False-*-reno-flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        adu_data = zip(tputs, durs, bytes)

    all_file_data['big_tp'].append([d[0] for d in adu_data
                                    if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['big_dur'].append([d[1] for d in adu_data
                                     if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['small_tp'].append([d[0] for d in adu_data
                                      if d[2] <= SMALL_FLOW_MAX_SIZE])
    all_file_data['small_dur'].append([d[1] for d in adu_data
                                       if d[2] <= SMALL_FLOW_MAX_SIZE])

    adu_resize_data = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-normal-*-'
                        'ADU-True-*-reno-flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        adu_resize_data = zip(tputs, durs, bytes)

    all_file_data['big_tp'].append([d[0] for d in adu_resize_data
                                    if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['big_dur'].append([d[1] for d in adu_resize_data
                                     if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['small_tp'].append([d[0] for d in adu_resize_data
                                      if d[2] <= SMALL_FLOW_MAX_SIZE])
    all_file_data['small_dur'].append([d[1] for d in adu_resize_data
                                       if d[2] <= SMALL_FLOW_MAX_SIZE])

    adu_ocs_data = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-normal-*-'
                        'ADU-True-*-ocs-flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        adu_ocs_data = zip(tputs, durs, bytes)

    all_file_data['big_tp'].append([d[0] for d in adu_ocs_data
                                    if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['big_dur'].append([d[1] for d in adu_ocs_data
                                     if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['small_tp'].append([d[0] for d in adu_ocs_data
                                      if d[2] <= SMALL_FLOW_MAX_SIZE])
    all_file_data['small_dur'].append([d[1] for d in adu_ocs_data
                                       if d[2] <= SMALL_FLOW_MAX_SIZE])

    fixed_data = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-fixed-*-flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        fixed_data = zip(tputs, durs, bytes)

    all_file_data['big_tp'].append([d[0] for d in fixed_data
                                    if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['big_dur'].append([d[1] for d in fixed_data
                                     if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['small_tp'].append([d[0] for d in fixed_data
                                      if d[2] <= SMALL_FLOW_MAX_SIZE])
    all_file_data['small_dur'].append([d[1] for d in fixed_data
                                       if d[2] <= SMALL_FLOW_MAX_SIZE])

    afd = all_file_data
    rows = []
    rows.append(['dur small 50 (ms)'] +
                [np.percentile(p, 50)/1000.0 for p in afd['small_dur']])
    rows.append(['dur small 99 (ms)'] +
                [np.percentile(p, 99)/1000.0 for p in afd['small_dur']])
    rows.append(['tp small 50 (Gbps)'] +
                [np.percentile(p, 50) for p in afd['small_tp']])
    rows.append(['tp small 99 (Gbps)'] +
                [np.percentile(p, 99) for p in afd['small_tp']])
    rows.append(['dur big 50 (ms)'] +
                [np.percentile(p, 50)/1000.0 for p in afd['big_dur']])
    rows.append(['dur big 99 (ms)'] +
                [np.percentile(p, 99)/1000.0 for p in afd['big_dur']])
    rows.append(['tp big 50 (Gbps)'] +
                [np.percentile(p, 50) for p in afd['big_tp']])
    rows.append(['tp big 99 (Gbps)'] +
                [np.percentile(p, 99) for p in afd['big_tp']])
    print tabulate(rows, headers=['', 'Today', 'Resize', 'OCS',
                                  'ADU', 'ADU+Resize', 'ADU+OCS', 'Fixed'],
                   numalign="right", floatfmt=".3f")
    print

    bytes = {}
    for fn in glob.glob(sys.argv[1] + '/*.counters.txt'):
        print fn
        key = 'QUEUE'
        if 'ADU' in fn:
            key = 'ADU'
        if 'fixed' in fn:
            key = 'FIXED'
        cir, p_up, p_down = eval(open(fn).read())
        cir = [int(c.split('\n')[-1]) for c in cir]
        p_up = [int(p.split('\n')[-1]) for p in p_up]
        p_down = [int(p.split('\n')[-1]) for p in p_down]
        print cir, p_up, p_down
        print sum(cir), sum(p_up), sum(p_down)
        bytes[key] = (cir, p_up, p_down)
    print sum(bytes['ADU'][0]) / float(sum(bytes['QUEUE'][0]))
    print sum(bytes['ADU'][1]) / float(sum(bytes['QUEUE'][1]))
    print sum(bytes['ADU'][2]) / float(sum(bytes['QUEUE'][2]))
    print
    print sum(bytes['ADU'][0]), sum(bytes['QUEUE'][0])
    print sum(bytes['ADU'][1]), sum(bytes['QUEUE'][1])
    print sum(bytes['ADU'][2]), sum(bytes['QUEUE'][2])

    graph([queue_data, adu_data, fixed_data], '', '')
    bytes_graph(bytes)
