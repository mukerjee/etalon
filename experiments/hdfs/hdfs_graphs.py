#!/usr/bin/env python

import sys
sys.path.insert(0, '..')
import glob
import numpy as np

from dotmap import DotMap
from simpleplotlib import plot
from parse_logs import parse_hdfs_logs, parse_hdfs_throughput

bytes_units = 2.0**-30

types = ['HDFS+static', 'HDFS+resize', 'HDFS+reTCP', 'reHDFS+static',
         'reHDFS+resize', 'reHDFS+reTCP']

fn_keys = {
    'normal-16-QUEUE-False': 'static',
    'normal-16-QUEUE-True-20000-reno': 'resize',
    'normal-16-QUEUE-True-20000-retcp': 'reTCP',
    'normal-16-ADU-False': 'adu',
    'normal-16-ADU-True-20000-reno': 'adu+resize',
    'normal-16-ADU-True-20000-retcp': 'adu+reTCP',
}

files = [
    '/tmp/*QUEUE-False*-HDFS-dfsioe',
    '/tmp/*QUEUE-True-20000-reno*-HDFS-dfsioe',
    '/tmp/*QUEUE-True-20000-retcp*-HDFS-dfsioe',
    '/tmp/*QUEUE-False*-reHDFS-dfsioe',
    '/tmp/*QUEUE-True-20000-reno*-reHDFS-dfsioe',
    '/tmp/*QUEUE-True-20000-retcp*-reHDFS-dfsioe',
]
    
files_short = [files[0], files[3]]
    

def get_default_plot_options(x, y):
    options = DotMap()
    options.plot_type = 'BAR'
    options.legend.options.labels = ['HDFS', 'HDFS + Resize',
                                     'HDFS + reTCP', 'reHDFS',
                                     'reHDFS + Resize',
                                     'reHDFS + reTCP']
    options.series.color_groups = [0, 0, 0, 1, 1, 1]
    options.legend.order = [0, 2, 4, 1, 3, 5]
    options.legend.options.fontsize = 19
    options.legend.options.ncol = 3
    options.x.ticks.major.show = False
    return options


def graph_wct(data):
    x = data
    y = [[float(j) / (len(x[i])-1) * 100 for j in xrange(len(x[i]))]
         for i in xrange(len(x))]
    
    options = get_default_plot_options(x, y)
    options.plot_type = 'LINE'
    options.legend.options.labels = ['HDFS', 'reHDFS']
    options.series_options = [DotMap(linewidth=5) for i in range(len(x))]
    options.output_fn = 'graphs/hdfs_writes_cdf.pdf'
    options.x.label.xlabel = 'HDFS write completion time (ms)'
    options.y.label.ylabel = 'CDF (%)'
    del options.series.color_groups
    del options.legend.options.ncol
    del options.x.ticks.major.show
    plot(x, y, options)


def graph_tail(data):
    x = np.array([[0] for i in xrange(len(data))])
    y = [np.percentile(d, 99) for d in data]
    
    options = get_default_plot_options(x, y)
    options.y.limits = [0, 1500]
    options.output_fn = 'graphs/hdfs_99th.pdf'
    options.y.label.ylabel = '99th percent. writes (ms)'
    options.y.ticks.major.show = False
    del options.legend.options.ncol
    del options.legend.order
    plot(x, y, options)


def graph_throughput(data):
    x = np.array([[0] for i in xrange(len(data))])
    y = data

    options = get_default_plot_options(x, y)
    options.horizontal_lines.lines = [80*8 + 10*8]
    options.legend.options.fontsize = 18
    options.y.label_offset = [-0.01, -.13]
    options.y.limits = [0, 1100]
    options.output_fn = 'graphs/hdfs_throughput.pdf'
    options.y.label.ylabel = 'Agg. tput. (Gbps)'
    options.y.ticks.major.show = False
    plot(x, y, options)


def bytes_graph():
    data = {}
    for fn in glob.glob(sys.argv[1] + '/*.counters.txt'):
        key = 'reHDFS+' if 'reHDFS' in fn else 'HDFS+'
        key += [k for n, k in fn_keys.items() if n in fn][0]
        c, p, _ = eval(open(fn).read())
        c = sum([int(b.split('\n')[-1]) * bytes_units for b in c])
        p = sum([int(b.split('\n')[-1]) * bytes_units for b in p])
        data[key] = p, c

    y = [data[t] for t in types]
    x = np.array([[0, 1] for i in xrange(len(y))])

    options = get_default_plot_options(x, y)
    options.bar_labels.show = False
    options.legend.options.fontsize = 18
    options.y.label_offset = [-.07, -.18]
    options.y.limits = [0, 40]
    options.x.ticks.major.labels = DotMap(
        text=['Packet', 'Circuit'])
    options.y.ticks.major.labels = DotMap(
        locations=[0, 5, 10, 15, 20, 25])
    options.output_fn = 'graphs/hdfs_utilization.pdf'
    options.x.label.xlabel = 'Which switch'
    options.y.label.ylabel = 'Bytes sent (GB)'
    plot(x, y, options)

if __name__ == '__main__':
    graph_wct([parse_hdfs_logs(sys.argv[1] + n) for n in files_short])
    graph_tail([parse_hdfs_logs(sys.argv[1] + n) for n in files])
    graph_throughput([parse_hdfs_throughput(sys.argv[1] + n) for n in files])
    bytes_graph()
