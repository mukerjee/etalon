#!/usr/bin/env python

from dotmap import DotMap

import sys
sys.path.insert(0, '../../')
sys.path.insert(0, '../')
sys.path.insert(0, '/Users/mukerjee/Dropbox/Research/simpleplotlib/')

import copy
import glob
import numpy as np
import matplotlib.pyplot as plt

from collections import defaultdict
from tabulate import tabulate
from get_throughput_and_latency import get_tput_and_dur
from parse_logs import parse_hadoop_logs, parse_total_writes, parse_job_completion_time, parse_throughput
from simpleplotlib import plot


def graph_wct(data):
    x = data
    y = []
    for i in xrange(len(x)):
        y.append([float(j) / (len(x[i])-1) * 100 for j in xrange(len(x[i]))])
    for p in x:
        print max(p)
    
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['HDFS', 'HDFS + Resize',
                                     'HDFS + reTCP', 'reHDFS',
                                     'reHDFS + Resize', 'reHDFS + reTCP',]
    if len(x) == 2:
        options.legend.options.labels = ['HDFS', 'reHDFS']
    options.legend.options.fontsize = 19
    options.series_options = [DotMap(color='C%d' % i, linewidth=5)
                              for i in range(len(x))]
    options.output_fn = 'graphs/hdfs_writes_cdf.pdf'
    options.x.label.xlabel = 'HDFS write completion time (ms)'
    options.y.label.ylabel = 'CDF (%)'
    plot(x, y, options)


def graph_tail(data):
    x = np.array([[0] for i in xrange(len(data))])
    y = [np.percentile(d, 99) for d in data]
    print y
    
    options = DotMap()
    options.plot_type = 'BAR'
    options.legend.options.labels = ['HDFS', 'HDFS + Resize',
                                     'HDFS + reTCP', 'reHDFS',
                                     'reHDFS + Resize', 'reHDFS + reTCP',]
    options.legend.options.fontsize = 19
    cm = plt.get_cmap('tab20c')
    options.series_options[0].color = cm(0)
    options.series_options[1].color = cm(1)
    options.series_options[2].color = cm(2)
    options.series_options[3].color = cm(4)
    options.series_options[4].color = cm(5)
    options.series_options[5].color = cm(6)
    options.legend.options.fontsize = 18
    # options.legend.options.ncol = 3
    # options.y.label_offset = [-0.01, -.13]
    options.y.limits = [0, 1500]
    options.y.ticks.major.show = False
    options.output_fn = 'graphs/hdfs_99th.pdf'
    options.y.label.ylabel = '99th percent. writes (ms)'
    plot(x, y, options)


def graph_total_time(data):
    x = np.array([[0] for i in xrange(len(data))])
    y = data
    print y
    
    options = DotMap()
    options.plot_type = 'BAR'
    options.legend.options.labels = ['HDFS', 'HDFS + Resize',
                                     'HDFS + reTCP', 'reHDFS',
                                     'reHDFS + Resize', 'reHDFS + reTCP',]
    options.legend.options.fontsize = 19
    options.output_fn = 'graphs/hdfs_total_time.pdf'
    options.y.label.ylabel = 'Total time writing (s)'
    plot(x, y, options)


def graph_job_completion_time(data):
    x = np.array([[0] for i in xrange(len(data))])
    y = data
    print y
    
    options = DotMap()
    options.plot_type = 'BAR'
    options.legend.options.labels = ['HDFS', 'HDFS + Resize',
                                     'HDFS + reTCP', 'reHDFS',
                                     'reHDFS + Resize', 'reHDFS + reTCP',]
    options.legend.options.fontsize = 19
    options.bar_labels.format_string = '%1.2f'
    options.bar_labels.options.fontsize = 25
    options.output_fn = 'graphs/hdfs_jct.pdf'
    options.y.label.ylabel = 'Job completion time (s)'
    plot(x, y, options)


def graph_throughput(data):
    x = np.array([[0] for i in xrange(len(data))])
    y = data
    print y

    options = DotMap()
    options.plot_type = 'BAR'
    options.legend.options.labels = ['HDFS', 'HDFS + Resize',
                                     'HDFS + reTCP', 'reHDFS',
                                     'reHDFS + Resize', 'reHDFS + reTCP',]
    options.legend.options.fontsize = 19
    options.horizontal_lines.lines = [80*8 + 10*8]
    options.output_fn = 'graphs/hdfs_throughput.pdf'
    cm = plt.get_cmap('tab20c')
    options.series_options[0].color = cm(0)
    options.series_options[1].color = cm(1)
    options.series_options[2].color = cm(2)
    options.series_options[3].color = cm(4)
    options.series_options[4].color = cm(5)
    options.series_options[5].color = cm(6)
    options.legend.order = [0, 2, 4, 1, 3, 5]
    options.legend.options.fontsize = 18
    options.legend.options.ncol = 3
    options.y.label_offset = [-0.01, -.13]
    options.y.limits = [0, 1100]
    # options.y.ticks.major.labels = DotMap(locations = [100, 300, 500, 700])
    options.y.ticks.major.show = False
    options.y.label.ylabel = 'Agg. tput. (Gbps)'
    plot(x, y, options)


def bytes_graph():
    bytes = {}
    for fn in glob.glob(sys.argv[1] + '/*.counters.txt'):
        print fn
        key = 'HADOOP_SDRT+' if 'SDRT' in fn else 'HADOOP+'
        if 'normal-16-ADU-True-20000-ocs' in fn:
            key += 'ADU+OCS'
        if 'normal-16-ADU-False' in fn:
            key += 'ADU'
        if 'normal-16-ADU-True-20000-reno' in fn:
            key += 'ADU+RESIZE'
        if 'normal-16-QUEUE-False' in fn:
            key += 'QUEUE'
        if 'normal-16-QUEUE-True-20000-ocs' in fn:
            key += 'OCS'
        if 'normal-16-QUEUE-True-20000-reno' in fn:
            key += 'RESIZE'
        print key
        cir, p_up, p_down = eval(open(fn).read())
        cir = [int(c.split('\n')[-1]) for c in cir]
        p_up = [int(p.split('\n')[-1]) for p in p_up]
        p_down = [int(p.split('\n')[-1]) for p in p_down]
        # print cir, p_up, p_down
        # print sum(cir), sum(p_up), sum(p_down)
        bytes[key] = (cir, p_up, p_down)

    y = [bytes['HADOOP+QUEUE'], bytes['HADOOP+RESIZE'], bytes['HADOOP+OCS'],
         bytes['HADOOP_SDRT+QUEUE'], bytes['HADOOP_SDRT+RESIZE'], bytes['HADOOP_SDRT+OCS'],]
    for i in xrange(len(y)):
        y[i] = [sum(y[i][1]), sum(y[i][0])]
    for i in xrange(len(y)):
        y[i] = [p / 1024.0 / 1024.0 / 1024.0 for p in y[i]]
        print sum(y[i])
    x = np.array([[0, 1] for i in xrange(len(y))])

    print y

    options = DotMap()
    options.plot_type = 'BAR'
    options.bar_labels.show = False
    options.legend.options.labels = ['HDFS', 'HDFS + Resize',
                                     'HDFS + reTCP', 'reHDFS',
                                     'reHDFS + Resize', 'reHDFS + reTCP',]
    options.series_options = [DotMap(color='C%d' % i) for i in range(len(x))]
    cm = plt.get_cmap('tab20c')
    # options.series_options[0].color = cm(0)
    # options.series_options[1].color = cm(1)
    # options.series_options[2].color = cm(2)
    # options.series_options[3].color = cm(4)
    # options.series_options[4].color = cm(5)
    # options.series_options[5].color = cm(6)
    # options.series_options[6].color = cm(8)
    # options.series_options[7].color = cm(9)
    # options.series_options[8].color = cm(12)
    # options.series_options[9].color = cm(13)
    # options.series_options[10].color = cm(14)
    # options.series_options[11].color = cm(15)
    options.series_options[0].color = cm(0)
    options.series_options[1].color = cm(1)
    options.series_options[2].color = cm(2)
    options.series_options[3].color = cm(4)
    options.series_options[4].color = cm(5)
    options.series_options[5].color = cm(6)
    options.legend.order = [0, 2, 4, 1, 3, 5]
    options.legend.options.fontsize = 18
    options.legend.options.ncol = 3
    options.y.label_offset = [-.07, -.18]
    # options.legend.options.loc = 'lower center'
    options.y.limits = [0, 40]
    options.x.ticks.major.show = False
    options.x.ticks.major.labels = DotMap(
        text=['Packet', 'Circuit'])
    options.y.ticks.major.labels = DotMap(locations = [0, 5, 10, 15, 20, 25])
                                          
    options.output_fn = 'graphs/hdfs_utilization.pdf'
    options.x.label.xlabel = 'Which switch'
    options.y.label.ylabel = 'Bytes sent (GB)'
    plot(x, y, options)

if __name__ == '__main__':
    names = [
        '/tmp/*QUEUE-False*-Hadoop-dfsioe',
        '/tmp/*QUEUE-True-20000-reno*-Hadoop-dfsioe',
        '/tmp/*QUEUE-True-20000-ocs*-Hadoop-dfsioe',
        # '/tmp/*ADU-False*-Hadoop-dfsioe',
        # '/tmp/*ADU-True-20000-reno*-Hadoop-dfsioe',
        # '/tmp/*ADU-True-20000-ocs*-Hadoop-dfsioe',
        '/tmp/*QUEUE-False*-Hadoop-SDRT-dfsioe',
        '/tmp/*QUEUE-True-20000-reno*-Hadoop-SDRT-dfsioe',
        '/tmp/*QUEUE-True-20000-ocs*-Hadoop-SDRT-dfsioe',
        # '/tmp/*ADU-False*-Hadoop-SDRT-dfsioe',
        # '/tmp/*ADU-True-20000-reno*-Hadoop-SDRT-dfsioe',
        # '/tmp/*ADU-True-20000-ocs*-Hadoop-SDRT-dfsioe',
    ]
    
    short_names = [
        '/tmp/*QUEUE-False*-Hadoop-dfsioe',
        '/tmp/*QUEUE-False*-Hadoop-SDRT-dfsioe',
    ]
    
    graph_wct([parse_hadoop_logs(sys.argv[1] + n) for n in short_names])
    graph_tail([parse_hadoop_logs(sys.argv[1] + n) for n in names])
    # graph_total_time([parse_total_writes(sys.argv[1] + n) for n in names])
    graph_job_completion_time([parse_job_completion_time(sys.argv[1] + n)
                               for n in names])
    graph_throughput([parse_throughput(sys.argv[1] + n) for n in names])
    
    bytes_graph()
