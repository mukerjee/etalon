#!/usr/bin/env python

import sys
sys.path.insert(0, '..')
import glob
import numpy as np

from collections import defaultdict
from dotmap import DotMap
from tabulate import tabulate
from simpleplotlib import plot
from parse_logs import parse_flowgrind_config

SMALL_FLOW_MAX_SIZE = 1000000
dur_units = 1e-3
bytes_units = 2.0**-30

types = ['static', 'resize', 'reTCP', 'adu', 'adu+resize', 'adu+reTCP',
         'fixed', 'packet10', 'packet20', 'packet40', 'packet80']

fn_keys = {
    'normal-16-QUEUE-False': 'static',
    'normal-16-QUEUE-True-20000-reno': 'resize',
    'normal-16-QUEUE-True-20000-retcp': 'reTCP',
    'normal-16-ADU-False': 'adu',
    'normal-16-ADU-True-20000-reno': 'adu+resize',
    'normal-16-ADU-True-20000-retcp': 'adu+reTCP',
    'fixed-16-': 'fixed',
    'fixed-128-': 'fixed_big',
    'no_circuit-128-QUEUE-False-12000-reno-0.0006-0.5-': 'packet10',
    '-1.0-': 'packet20',
    '-2.0-': 'packet40',
    '-4.0-': 'packet80',
    '-4.5-': 'packet90',
}

files = {
    'static': '/*-normal-*-QUEUE-False-*-reno-*flowgrind.txt',
    'resize': '/*-normal-*-QUEUE-True-*-reno-*flowgrind.txt',
    'reTCP': '/*-normal-*-QUEUE-True-*-retcp-*flowgrind.txt',
    'adu': '/*-normal-*-ADU-False-*-reno-*flowgrind.txt',
    'adu+resize': '/*-normal-*-ADU-True-*-reno-*flowgrind.txt',
    'adu+reTCP': '/*-normal-*-ADU-True-*-retcp-*flowgrind.txt',
    'fixed': '/*-fixed-16-*flowgrind.txt',
    'packet10': '/*-no_circuit-*-0.5-*flowgrind.txt',
    'packet20': '/*-no_circuit-*-1.0-*flowgrind.txt',
    'packet40': '/*-no_circuit-*-2.0-*flowgrind.txt',
    'packet80': '/*-no_circuit-*-4.0-*flowgrind.txt',
}

all_file_data = defaultdict(list)


def get_default_plot_options(x, y):
    options = DotMap()
    options.plot_type = 'LINE'
    options.rcParams['figure.figsize'] = [6.0, 4.0]
    options.legend.options.fontsize = 19
    options.legend.options.labels = ['ToR VOQ', 'ADU', 'Fixed Sched.']
    options.legend.styles = [DotMap(zorder=i, linewidth=5)
                             for i in range(len(x))]
    options.series_options = [DotMap(zorder=i, linewidth=5)
                              for i in range(len(x))]
    options.series_options[2].linewidth = 10
    options.series_options[1].zorder = 5
    options.y.label.ylabel = 'CDF (%)'
    return options


def get_data(type):
    data = [(0, 0, 0), (0, 0, SMALL_FLOW_MAX_SIZE+1)]
    if glob.glob(sys.argv[1] + files[type]):
        fn = glob.glob(sys.argv[1] + files[type])[0]
        tputs, durs, bytes = parse_flowgrind_config(fn)
        data = zip(tputs, durs, bytes)

        all_file_data['big_tp'].append([d[0] for d in data
                                        if d[2] > SMALL_FLOW_MAX_SIZE])
        all_file_data['big_dur'].append([d[1] for d in data
                                         if d[2] > SMALL_FLOW_MAX_SIZE])
        all_file_data['small_tp'].append([d[0] for d in data
                                          if d[2] <= SMALL_FLOW_MAX_SIZE])
        all_file_data['small_dur'].append([d[1] for d in data
                                           if d[2] <= SMALL_FLOW_MAX_SIZE])
    return data


def graph_small_durs(data):
    x = [sorted([p[1] * dur_units for p in d if p[2] <=
                 SMALL_FLOW_MAX_SIZE]) for d in data]
    y = [[float(j) / (len(x[i])-1) * 100 for j in xrange(len(x[i]))]
         for i in xrange(len(x))]
    
    options = get_default_plot_options(x, y)
    options.x.limits = [0, 100]
    options.output_fn = 'graphs/mice_fct_cdf.pdf'
    options.x.label.xlabel = 'Flow-completion time (ms)'
    plot(x, y, options)


def graph_big_durs(data):
    x = [sorted([p[1] * dur_units for p in d if p[2] >
                 SMALL_FLOW_MAX_SIZE]) for d in data]
    y = [[float(j) / (len(x[i])-1) * 100 for j in xrange(len(x[i]))]
         for i in xrange(len(x))]

    options = get_default_plot_options(x, y)
    options.output_fn = 'graphs/elephant_fct_cdf.pdf'
    options.x.label.xlabel = 'Flow-completion time (ms)'
    plot(x, y, options)


def graph_small_tp(data):
    x = [sorted([p[0] for p in d if p[2] <= SMALL_FLOW_MAX_SIZE])
         for d in data]
    y = [[float(j) / (len(x[i])-1) * 100 for j in xrange(len(x[i]))]
         for i in xrange(len(x))]
    
    options = get_default_plot_options(x, y)
    options.x.limits = [0, 10]
    options.output_fn = 'graphs/mice_tp_cdf.pdf'
    options.x.label.xlabel = 'Throughput (Gbps)'
    plot(x, y, options)


def graph_big_tp(data):
    x = [sorted([p[0] for p in d if p[2] > SMALL_FLOW_MAX_SIZE])
         for d in data]
    y = [[float(j) / (len(x[i])-1) * 100 for j in xrange(len(x[i]))]
         for i in xrange(len(x))]
    
    options = get_default_plot_options(x, y)
    options.x.limits = [0, 10]
    options.output_fn = 'graphs/elephant_tp_cdf.pdf'
    options.x.label.xlabel = 'Throughput (Gbps)'
    plot(x, y, options)


def bytes_graph():
    data = {}
    for fn in glob.glob(sys.argv[1] + '/*.counters.txt'):
        key = [key for n, key in fn_keys.items() if n in fn][0]
        c, p, _ = eval(open(fn).read())
        c = sum([int(b.split('\n')[-1]) * bytes_units for b in c])
        p = sum([int(b.split('\n')[-1]) * bytes_units for b in p])
        data[key] = p, c

    y = [data[t] for t in types]
    x = np.array([[0, 1] for i in xrange(len(y))])

    options = DotMap()
    options.plot_type = 'BAR'
    options.bar_labels.show = False
    options.legend.options.labels = ['ToR VOQ', 'VOQ + Resize',
                                     'VOQ + reTCP', 'ADU', 'ADU + Resize',
                                     'ADU + reTCP', 'Fixed Sched.',
                                     'Packet (10G)', 'Packet (20G)',
                                     'Packet (40G)', 'Packet (80G)']
    options.legend.options.fontsize = 12
    options.legend.options.ncol = 4
    options.series.color_groups = [0, 0, 0, 1, 1, 1, 2, 3, 3, 3, 3]
    options.y.limits = [0, 85]
    options.y.label_offset = [-.07, -.11]
    options.x.ticks.major.show = False
    options.x.ticks.major.labels = DotMap(text=['Packet', 'Circuit'])
    options.y.ticks.major.labels = DotMap(locations=[0, 15, 30, 45, 60])
    options.output_fn = 'graphs/utilization.pdf'
    options.x.label.xlabel = 'Which switch'
    options.y.label.ylabel = 'Bytes sent (GB)'
    plot(x, y, options)

if __name__ == '__main__':
    data = [get_data(t) for t in types]

    afd = all_file_data
    rows = []
    rows.append(['Median small FCT (ms)'] +
                [np.percentile(p, 50) * dur_units for p in afd['small_dur']])
    rows.append(['99th small FCT (ms)'] +
                [np.percentile(p, 99) * dur_units for p in afd['small_dur']])
    rows.append(['Median big FCT (ms)'] +
                [np.percentile(p, 50) * dur_units for p in afd['big_dur']])
    rows.append(['99th big FCT (ms)'] +
                [np.percentile(p, 99) * dur_units for p in afd['big_dur']])
    header_strings = ['', 'ToR VOQ', 'VOQ + Resize', 'VOQ + reTCP',
                      'ADU', 'ADU + Resize', 'ADU + reTCP',
                      'Fixed Sched.', 'Packet (10)', 'Packet (20)',
                      'Packet (40)', 'Packet (80)']
    print tabulate(rows, headers=header_strings, numalign="right",
                   floatfmt=".2f")
    print tabulate(rows, headers=header_strings, numalign="right",
                   floatfmt=".2f", tablefmt="latex")

    data = [data[types.index('static')], data[types.index('adu')],
            data[types.index('fixed')]]
    graph_small_durs(data)
    graph_big_durs(data)
    graph_small_tp(data)
    graph_big_tp(data)
    try:
        bytes_graph()
    except:
        pass
