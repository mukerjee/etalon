#!/usr/bin/env python

from dotmap import DotMap

import sys
sys.path.insert(0, '../../')
sys.path.insert(0, '/Users/mukerjee/Dropbox/Research/simpleplotlib/')

import copy
import glob
import numpy as np
import matplotlib.pyplot as plt

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
         sorted(zip(*data[2])[1]), sorted(zip(*data[3])[1])]
    y = [[float(i) / (len(x[0])-1) * 100 for i in xrange(len(x[0]))]]
    y.append([float(i) / (len(x[1])-1) * 100 for i in xrange(len(x[1]))])
    y.append([float(i) / (len(x[2])-1) * 100 for i in xrange(len(x[2]))])
    y.append([float(i) / (len(x[3])-1) * 100 for i in xrange(len(x[3]))])
    print
    print len(x[0])
    
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['ToR VOQ (16)', 'ADU (16)',
                                     'Fixed (16)', 'Fixed (128)']
    options.legend.options.fontsize = 19
    options.legend.options.labels = ['ToR VOQ', 'ADU',
                                     'Fixed Sched.']
    options.series_options = [DotMap(color='C%d' % i, zorder=i, linewidth=5)
                              for i in range(len(x))]
    options.series_options[2].linewidth = 10
    options.series_options[1].zorder = 10
    options.legend.styles = [DotMap(color='C%d' % i, zorder=i, linewidth=5)
                             for i in range(len(x))]
    options.x.limits = [0, 100]
    options.output_fn = 'graphs/mice_fct_cdf.pdf'
    options.x.label.xlabel = 'Flow completion time (ms)'
    options.y.label.ylabel = 'CDF (%)'
    options.rcParams['figure.figsize'] = [6.0, 4.0]
    # plot(x, y, options)
    plot(x[:3], y[:3], options)


def graph_big_durs(d2, x_label, fn):
    data = copy.deepcopy(d2)
    for i in xrange(len(data)):
        data[i] = [[d[0], d[1] / 1000.0, d[2]]
                   for d in data[i] if d[2] > SMALL_FLOW_MAX_SIZE]
    x = [sorted(zip(*data[0])[1]), sorted(zip(*data[1])[1]),
         sorted(zip(*data[2])[1]), sorted(zip(*data[3])[1])]
    y = [[float(i) / (len(x[0])-1) * 100 for i in xrange(len(x[0]))]]
    y.append([float(i) / (len(x[1])-1) * 100 for i in xrange(len(x[1]))])
    y.append([float(i) / (len(x[2])-1) * 100 for i in xrange(len(x[2]))])
    y.append([float(i) / (len(x[3])-1) * 100 for i in xrange(len(x[3]))])
    print
    print len(x[0])

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['ToR VOQ (16)', 'ADU (16)',
                                     'Fixed (16)', 'Fixed (128)']
    options.legend.options.fontsize = 19
    options.legend.options.labels = ['ToR VOQ', 'ADU',
                                     'Fixed Sched.']
    options.series_options = [DotMap(color='C%d' % i, zorder=i, linewidth=5)
                              for i in range(len(x))]
    options.series_options[2].linewidth = 10
    options.series_options[1].zorder = 10
    options.legend.styles = [DotMap(color='C%d' % i, zorder=i, linewidth=5)
                             for i in range(len(x))]
    options.output_fn = 'graphs/elephant_fct_cdf.pdf'
    options.x.label.xlabel = 'Flow completion time (ms)'
    options.y.label.ylabel = 'CDF (%)'
    options.rcParams['figure.figsize'] = [6.0, 4.0]
    # plot(x, y, options)
    plot(x[:3], y[:3], options)


def graph_small_tp(d2, x_label, fn):
    data = copy.deepcopy(d2)
    for i in xrange(len(data)):
        data[i] = [d for d in data[i] if d[2] <= SMALL_FLOW_MAX_SIZE]
    x = [sorted(zip(*data[0])[0]), sorted(zip(*data[1])[0]),
         sorted(zip(*data[2])[0]), sorted(zip(*data[3])[0])]
    y = [[float(i) / (len(x[0])-1) * 100 for i in xrange(len(x[0]))]]
    y.append([float(i) / (len(x[1])-1) * 100 for i in xrange(len(x[1]))])
    y.append([float(i) / (len(x[2])-1) * 100 for i in xrange(len(x[2]))])
    y.append([float(i) / (len(x[3])-1) * 100 for i in xrange(len(x[3]))])
    y.append([float(i) / (len(x[3])-1) * 100 for i in xrange(len(x[3]))])
    print
    print len(x[0])

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['ToR VOQ (16)', 'ADU (16)',
                                     'Fixed (16)', 'Fixed (128)']
    options.legend.options.fontsize = 19
    options.legend.options.labels = ['ToR VOQ', 'ADU',
                                     'Fixed Sched.']
    options.series_options = [DotMap(color='C%d' % i, zorder=i, linewidth=5)
                              for i in range(len(x))]
    options.series_options[2].linewidth = 10
    options.series_options[1].zorder = 10
    options.legend.styles = [DotMap(color='C%d' % i, zorder=i, linewidth=5)
                             for i in range(len(x))]
    options.x.limits = [0, 10]
    options.output_fn = 'graphs/mice_tp_cdf.pdf'
    options.x.label.xlabel = 'Throughput (Gbps)'
    options.y.label.ylabel = 'CDF (%)'
    options.rcParams['figure.figsize'] = [6.0, 4.0]
    # plot(x, y, options)
    plot(x[:3], y[:3], options)


def graph_big_tp(d2, x_label, fn):
    data = copy.deepcopy(d2)
    for i in xrange(len(data)):
        data[i] = [d for d in data[i] if d[2] > SMALL_FLOW_MAX_SIZE]
    x = [sorted(zip(*data[0])[0]), sorted(zip(*data[1])[0]),
         sorted(zip(*data[2])[0]), sorted(zip(*data[3])[0])]
    y = [[float(i) / (len(x[0])-1) * 100 for i in xrange(len(x[0]))]]
    y.append([float(i) / (len(x[1])-1) * 100 for i in xrange(len(x[1]))])
    y.append([float(i) / (len(x[2])-1) * 100 for i in xrange(len(x[2]))])
    y.append([float(i) / (len(x[3])-1) * 100 for i in xrange(len(x[3]))])
    print
    print len(x[0])
    
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['ToR VOQ (16)', 'ADU (16)',
                                     'Fixed (16)', 'Fixed (128)']
    options.legend.options.fontsize = 19
    options.legend.options.labels = ['ToR VOQ', 'ADU',
                                     'Fixed Sched.']
    options.series_options = [DotMap(color='C%d' % i, zorder=i, linewidth=5)
                              for i in range(len(x))]
    options.series_options[2].linewidth = 10
    options.series_options[1].zorder = 5
    options.legend.styles = [DotMap(color='C%d' % i, zorder=i, linewidth=5)
                             for i in range(len(x))]
    options.x.limits = [0, 10]
    options.output_fn = 'graphs/elephant_tp_cdf.pdf'
    options.x.label.xlabel = 'Throughput (Gbps)'
    options.y.label.ylabel = 'CDF (%)'
    options.rcParams['figure.figsize'] = [6.0, 4.0]
    # plot(x, y, options)
    plot(x[:3], y[:3], options)


def bytes_graph(d2):
    data = copy.deepcopy(d2)
    # y = [data['QUEUE'], data['RESIZE'], data['OCS'], data['ADU'],
    #      data['ADU+RESIZE'], data['ADU+OCS'], data['FIXED'],
    #      data['FIXED_BIG'], data['PACKET_0.5'], data['PACKET_1.0'],
    #      data['PACKET_2.0'], data['PACKET_4.0']]
    y = [data['QUEUE'], data['RESIZE'], data['OCS'], data['ADU'],
         data['ADU+RESIZE'], data['ADU+OCS'], data['FIXED'],
         data['PACKET_0.5'], data['PACKET_1.0'],
         data['PACKET_2.0'], data['PACKET_4.0']]
    for i in xrange(len(y)):
        y[i] = [sum(y[i][1]), sum(y[i][0])]
    # y = [[sum(data['QUEUE'][0]), sum(data['QUEUE'][1])],
    #      [sum(data['ADU'][0]), sum(data['ADU'][1])],
    #      [sum(data['FIXED'][0]), sum(data['FIXED'][1])],
    #      [sum(data['FIXED_BIG'][0]), sum(data['FIXED_BIG'][1])]]
    for i in xrange(len(y)):
        y[i] = [p / 1024.0 / 1024.0 / 1024.0 for p in y[i]]
    x = np.array([[0, 1] for i in xrange(len(y))])

    print x, y

    options = DotMap()
    options.plot_type = 'BAR'
    options.bar_labels.show = False
    options.legend.options.labels = ['ToR VOQ', 'VOQ + Resize',
                                     'VOQ + reTCP', 'ADU', 'ADU + Resize',
                                     'ADU + reTCP', 'Fixed Sched.',
                                     # 'Fixed (128)', 'Packet (10G)',
                                     'Packet (10G)',
                                     'Packet (20G)', 'Packet (40G)',
                                     'Packet (80G)']
    options.series_options = [DotMap(color='C%d' % i) for i in range(len(x))]
    cm = plt.get_cmap('tab20c')
    options.series_options[0].color = cm(0)
    options.series_options[1].color = cm(1)
    options.series_options[2].color = cm(2)
    options.series_options[3].color = cm(4)
    options.series_options[4].color = cm(5)
    options.series_options[5].color = cm(6)
    options.series_options[6].color = cm(8)
    # options.series_options[7].color = cm(9)
    # options.series_options[8].color = cm(12)
    # options.series_options[9].color = cm(13)
    # options.series_options[10].color = cm(14)
    # options.series_options[11].color = cm(15)
    options.series_options[7].color = cm(12)
    options.series_options[8].color = cm(13)
    options.series_options[9].color = cm(14)
    options.series_options[10].color = cm(15)
    options.legend.options.fontsize = 12
    options.legend.options.ncol = 4
    # options.legend.options.loc = 'lower center'
    options.y.limits = [0, 85]
    options.y.label_offset = [-.07, -.11]
    options.x.ticks.major.show = False
    options.x.ticks.major.labels = DotMap(
        # locations = [.8, 1.6],
        text = ['Packet', 'Circuit'])
    options.y.ticks.major.labels = DotMap(locations = [0, 15, 30, 45, 60])
                                          
    options.output_fn = 'graphs/utilization.pdf'
    options.x.label.xlabel = 'Which switch'
    options.y.label.ylabel = 'Bytes sent (GB)'
    plot(x, y, options)

if __name__ == '__main__':
    all_file_data = defaultdict(list)
    queue_data = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-normal-*-'
                        'QUEUE-False-*-reno-*flowgrind.txt'):
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
    print '# big flows', len(all_file_data['big_tp'][0])
    print '# small flows', len(all_file_data['small_tp'][0])

    resize_data = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-normal-*-'
                        'QUEUE-True-*-reno-*flowgrind.txt'):
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
                        'QUEUE-True-*-ocs-*flowgrind.txt'):
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
                        'ADU-False-*-reno-*flowgrind.txt'):
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
                        'ADU-True-*-reno-*flowgrind.txt'):
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
                        'ADU-True-*-ocs-*flowgrind.txt'):
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
    for fn in glob.glob(sys.argv[1] + '/*-fixed-16-*flowgrind.txt'):
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

    fixed_data_big = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-fixed-128-*flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        fixed_data_big = zip(tputs, durs, bytes)

    all_file_data['big_tp'].append([d[0] for d in fixed_data_big
                                    if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['big_dur'].append([d[1] for d in fixed_data_big
                                     if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['small_tp'].append([d[0] for d in fixed_data_big
                                      if d[2] <= SMALL_FLOW_MAX_SIZE])
    all_file_data['small_dur'].append([d[1] for d in fixed_data_big
                                       if d[2] <= SMALL_FLOW_MAX_SIZE])
    
    packet_10 = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-no_circuit-*-0.5-*flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        packet_10 = zip(tputs, durs, bytes)

    all_file_data['big_tp'].append([d[0] for d in packet_10
                                    if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['big_dur'].append([d[1] for d in packet_10
                                     if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['small_tp'].append([d[0] for d in packet_10
                                      if d[2] <= SMALL_FLOW_MAX_SIZE])
    all_file_data['small_dur'].append([d[1] for d in packet_10
                                       if d[2] <= SMALL_FLOW_MAX_SIZE])

    packet_20 = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-no_circuit-*-1.0-*flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        packet_20 = zip(tputs, durs, bytes)

    all_file_data['big_tp'].append([d[0] for d in packet_20
                                    if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['big_dur'].append([d[1] for d in packet_20
                                     if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['small_tp'].append([d[0] for d in packet_20
                                      if d[2] <= SMALL_FLOW_MAX_SIZE])
    all_file_data['small_dur'].append([d[1] for d in packet_20
                                       if d[2] <= SMALL_FLOW_MAX_SIZE])

    packet_40 = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-no_circuit-*-2.0-*flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        packet_40 = zip(tputs, durs, bytes)

    all_file_data['big_tp'].append([d[0] for d in packet_40
                                    if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['big_dur'].append([d[1] for d in packet_40
                                     if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['small_tp'].append([d[0] for d in packet_40
                                      if d[2] <= SMALL_FLOW_MAX_SIZE])
    all_file_data['small_dur'].append([d[1] for d in packet_40
                                       if d[2] <= SMALL_FLOW_MAX_SIZE])

    packet_80 = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    for fn in glob.glob(sys.argv[1] + '/*-no_circuit-*-4.0-*flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        packet_80 = zip(tputs, durs, bytes)

    all_file_data['big_tp'].append([d[0] for d in packet_80
                                    if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['big_dur'].append([d[1] for d in packet_80
                                     if d[2] > SMALL_FLOW_MAX_SIZE])
    all_file_data['small_tp'].append([d[0] for d in packet_80
                                      if d[2] <= SMALL_FLOW_MAX_SIZE])
    all_file_data['small_dur'].append([d[1] for d in packet_80
                                       if d[2] <= SMALL_FLOW_MAX_SIZE])

    # packet_90 = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    # for fn in glob.glob(sys.argv[1] + '/*-no_circuit-*-4.5-*flowgrind.txt'):
    #     tputs, durs, bytes = get_tput_and_dur(fn)
    #     packet_90 = zip(tputs, durs, bytes)

    # all_file_data['big_tp'].append([d[0] for d in packet_90
    #                                 if d[2] > SMALL_FLOW_MAX_SIZE])
    # all_file_data['big_dur'].append([d[1] for d in packet_90
    #                                  if d[2] > SMALL_FLOW_MAX_SIZE])
    # all_file_data['small_tp'].append([d[0] for d in packet_90
    #                                   if d[2] <= SMALL_FLOW_MAX_SIZE])
    # all_file_data['small_dur'].append([d[1] for d in packet_90
    #                                    if d[2] <= SMALL_FLOW_MAX_SIZE])
    
    afd = all_file_data
    rows = []
    rows.append(['Median small FCT (ms)'] +
                [np.percentile(p, 50)/1000.0 for p in afd['small_dur']])
    rows.append(['99th small FCT (ms)'] +
                [np.percentile(p, 99)/1000.0 for p in afd['small_dur']])
    rows.append(['Median big FCT (ms)'] +
                [np.percentile(p, 50)/1000.0 for p in afd['big_dur']])
    rows.append(['99th big FCT (ms)'] +
                [np.percentile(p, 99)/1000.0 for p in afd['big_dur']])
    rows.append(['Median small TP (Gbps)'] +
                [np.percentile(p, 50) for p in afd['small_tp']])
    rows.append(['1st small TP (Gbps)'] +
                [np.percentile(p, 1) for p in afd['small_tp']])
    rows.append(['Median big TP (Gbps)'] +
                [np.percentile(p, 50) for p in afd['big_tp']])
    rows.append(['1st small TP (Gbps)'] +
                [np.percentile(p, 1) for p in afd['big_tp']])
    # print tabulate(rows, headers=['', 'ToR VOQ', 'VOQ + Resize', 'VOQ + \\tcp_ocs',
    #                               'ADU', 'ADU + Resize', 'ADU + \\tcp_ocs', 'Fixed (16)',
    #                               'Fixed (128)', 'Packet (10)', 'Packet (20)', 'Packet (40)',
    #                               'Packet (80)'], #, 'Packet (90)'],
    #                numalign="right", floatfmt=".2f")
    # print tabulate(rows, headers=['', 'ToR VOQ', 'VOQ + Resize', 'VOQ + \\tcp_ocs',
    #                               'ADU', 'ADU + Resize', 'ADU + \\tcp_ocs', 'Fixed (16)',
    #                               'Fixed (128)', 'Packet (10)', 'Packet (20)', 'Packet (40)',
    #                               'Packet (80)'], #, 'Packet (90)'],
    #                numalign="right", floatfmt=".2f", tablefmt="latex")
    print tabulate(rows, headers=['', 'ToR VOQ', 'VOQ + Resize', 'VOQ + \\tcp_ocs',
                                  'ADU', 'ADU + Resize', 'ADU + \\tcp_ocs', 'Fixed Sched.',
                                  'Packet (10)', 'Packet (20)', 'Packet (40)',
                                  'Packet (80)'], #, 'Packet (90)'],
                   numalign="right", floatfmt=".2f")
    print tabulate(rows, headers=['', 'ToR VOQ', 'VOQ + Resize', 'VOQ + \\tcp_ocs',
                                  'ADU', 'ADU + Resize', 'ADU + \\tcp_ocs', 'Fixed Sched.',
                                  'Packet (10)', 'Packet (20)', 'Packet (40)',
                                  'Packet (80)'], #, 'Packet (90)'],
                   numalign="right", floatfmt=".2f", tablefmt="latex")
    print

    bytes = {}
    for fn in glob.glob(sys.argv[1] + '/*.counters.txt'):
        print fn
        if 'normal-16-ADU-True-20000-ocs' in fn:
            key = 'ADU+OCS'
        if 'fixed-16-' in fn:
            key = 'FIXED'
        if 'normal-16-ADU-False' in fn:
            key = 'ADU'
        if 'normal-16-ADU-True-20000-reno' in fn:
            key = 'ADU+RESIZE'
        if 'normal-16-QUEUE-False' in fn:
            key = 'QUEUE'
        if 'fixed-128-' in fn:
            key = 'FIXED_BIG'
        if 'no_circuit' in fn:
            if '-0.5-' in fn:
                key = 'PACKET_0.5'
            if '-1.0-' in fn:
                key = 'PACKET_1.0'
            if '-2.0-' in fn:
                key = 'PACKET_2.0'
            if '-4.0-' in fn:
                key = 'PACKET_4.0'
            if '-4.5-' in fn:
                key = 'PACKET_4.5'
        if 'normal-16-QUEUE-True-20000-ocs' in fn:
            key = 'OCS'
        if 'normal-16-QUEUE-True-20000-reno' in fn:
            key = 'RESIZE'
        print key
        cir, p_up, p_down = eval(open(fn).read())
        cir = [int(c.split('\n')[-1]) for c in cir]
        p_up = [int(p.split('\n')[-1]) for p in p_up]
        p_down = [int(p.split('\n')[-1]) for p in p_down]
        # print cir, p_up, p_down
        # print sum(cir), sum(p_up), sum(p_down)
        bytes[key] = (cir, p_up, p_down)
    print sum(bytes['ADU'][0]) / float(sum(bytes['QUEUE'][0]))
    print sum(bytes['ADU'][1]) / float(sum(bytes['QUEUE'][1]))
    print sum(bytes['ADU'][2]) / float(sum(bytes['QUEUE'][2]))
    print
    print sum(bytes['ADU'][0]), sum(bytes['QUEUE'][0])
    print sum(bytes['ADU'][1]), sum(bytes['QUEUE'][1])
    print sum(bytes['ADU'][2]), sum(bytes['QUEUE'][2])

    graph([queue_data, adu_data, fixed_data, fixed_data_big], '', '')
    bytes_graph(bytes)
