#!/usr/bin/env python

from dotmap import DotMap

import sys
sys.path.insert(0, '../../')
sys.path.insert(0, '/Users/mukerjee/Dropbox/Research/simpleplotlib/')

import copy
import glob
import numpy as np

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
    x = [sorted(zip(*data[0])[1]), sorted(zip(*data[1])[1])]
    y = [[float(i) / (len(x[0])-1) for i in xrange(len(x[0]))]]
    y.append([float(i) / (len(x[1])-1) for i in xrange(len(x[1]))])
    print
    print len(x[0])
    # print 'durations:', x
    # print 'cdf:', y

    print 'dur small 50', np.percentile(x[0], 50), np.percentile(x[1], 50)
    print 'dur small 99', np.percentile(x[0], 99), np.percentile(x[1], 99)
    
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Today', 'ADU']
    options.legend.options.fontsize = 12
    # options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1)
    #                           for i in range(len(x))]
    options.x.limits = [0, 100]
    # options.x.limits = [0, 5000]
    options.output_fn = 'graphs/mice_fct_cdf.pdf'
    options.x.label.xlabel = 'Flow completion time (ms)'
    options.y.label.ylabel = 'CDF'
    plot(x, y, options)


def graph_big_durs(d2, x_label, fn):
    data = copy.deepcopy(d2)
    for i in xrange(len(data)):
        data[i] = [[d[0], d[1] / 1000.0, d[2]]
                   for d in data[i] if d[2] > SMALL_FLOW_MAX_SIZE]
    x = [sorted(zip(*data[0])[1]), sorted(zip(*data[1])[1])]
    y = [[float(i) / (len(x[0])-1) for i in xrange(len(x[0]))]]
    y.append([float(i) / (len(x[1])-1) for i in xrange(len(x[1]))])
    print
    print len(x[0])
    # print 'durations:', x
    # print 'cdf:', y

    print 'dur big 50', np.percentile(x[0], 50), np.percentile(x[1], 50)
    print 'dur big 99', np.percentile(x[0], 99), np.percentile(x[1], 99)

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Today', 'ADU']
    options.legend.options.fontsize = 12
    # options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1)
    #                           for i in range(len(x))]
    # options.x.limits = [0, 20000]
    # options.x.log = True
    options.output_fn = 'graphs/elephant_fct_cdf.pdf'
    options.x.label.xlabel = 'Flow completion time (ms)'
    options.y.label.ylabel = 'CDF'
    plot(x, y, options)


def graph_small_tp(d2, x_label, fn):
    data = copy.deepcopy(d2)
    for i in xrange(len(data)):
        data[i] = [d for d in data[i] if d[2] <= SMALL_FLOW_MAX_SIZE]
    x = [sorted(zip(*data[0])[0]), sorted(zip(*data[1])[0])]
    y = [[float(i) / (len(x[0])-1) for i in xrange(len(x[0]))]]
    y.append([float(i) / (len(x[1])-1) for i in xrange(len(x[1]))])
    print
    print len(x[0])
    # print 'durations:', x
    # print 'cdf:', y

    print 'tp small 50', np.percentile(x[0], 50), np.percentile(x[1], 50)
    print 'tp small 99', np.percentile(x[0], 99), np.percentile(x[1], 99)

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Today', 'ADU']
    options.legend.options.fontsize = 12
    # options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1)
    #                           for i in range(len(x))]
    options.x.limits = [0, 10]
    options.output_fn = 'graphs/mice_tp_cdf.pdf'
    options.x.label.xlabel = 'Throughput (Gbps)'
    options.y.label.ylabel = 'CDF'
    plot(x, y, options)


def graph_big_tp(d2, x_label, fn):
    data = copy.deepcopy(d2)
    for i in xrange(len(data)):
        data[i] = [d for d in data[i] if d[2] > SMALL_FLOW_MAX_SIZE]
    x = [sorted(zip(*data[0])[0]), sorted(zip(*data[1])[0])]
    y = [[float(i) / (len(x[0])-1) for i in xrange(len(x[0]))]]
    y.append([float(i) / (len(x[1])-1) for i in xrange(len(x[1]))])
    print
    print len(x[0])
    # print 'durations:', x
    # print 'cdf:', y

    print 'tp big 50', np.percentile(x[0], 50), np.percentile(x[1], 50)
    print 'tp big 99', np.percentile(x[0], 99), np.percentile(x[1], 99)
    
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Today', 'ADU']
    options.legend.options.fontsize = 12
    # options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1)
    #                           for i in range(len(x))]
    options.x.limits = [0, 10]
    # options.x.log = True
    options.output_fn = 'graphs/elephant_tp_cdf.pdf'
    options.x.label.xlabel = 'Throughput (Gbps)'
    options.y.label.ylabel = 'CDF'
    plot(x, y, options)


def bytes_graph(d2):
    data = copy.deepcopy(d2)
    x = np.array([[0, 1], [0, 1]])
    y = [[sum(data['QUEUE'][0]), sum(data['QUEUE'][1])],
         [sum(data['ADU'][0]), sum(data['ADU'][1])]]
    for i in xrange(len(y)):
        y[i] = [p / 1024.0 / 1024.0 / 1024.0 for p in y[i]]

    print x, y

    options = DotMap()
    options.plot_type = 'BAR'
    options.legend.options.labels = ['Today', 'ADU']
    options.legend.options.fontsize = 12
    # options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1)
    #                           for i in range(len(x))]
    # options.x.limits = [0, 10]
    # options.x.log = True
    options.output_fn = 'graphs/utilization.pdf'
    options.x.label.xlabel = ''
    options.y.label.ylabel = 'Bytes sent (GB)'
    plot(x, y, options)

if __name__ == '__main__':
    for fn in glob.glob(sys.argv[1] + '/*-normal-*-'
                        'QUEUE-False-*-reno-flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        queue_data = zip(tputs, durs, bytes)

    for fn in glob.glob(sys.argv[1] + '/*-normal-*-'
                        'ADU-False-*-reno-flowgrind.txt'):
        tputs, durs, bytes = get_tput_and_dur(fn)
        adu_data = zip(tputs, durs, bytes)

    bytes = {}
    for fn in glob.glob(sys.argv[1] + '/*.counters.txt'):
        print fn
        key = 'QUEUE'
        if 'ADU' in fn:
            key = 'ADU'
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

    graph([queue_data, adu_data], '', '')
    bytes_graph(bytes)
