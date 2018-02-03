#!/usr/bin/env python

from dotmap import DotMap

import sys
sys.path.insert(0, '../../')
sys.path.insert(0, '/Users/mukerjee/Dropbox/Research/simpleplotlib/')

import shelve
import os
import glob
import numpy as np

from collections import defaultdict
from get_throughput_and_latency import get_tput_and_lat
from simpleplotlib import plot

SR = (1, 2)
link_delays = [1e-4, 2e-4, 3e-4, 4e-4, 6e-4, 8e-4, 12e-4, 16e-4, 24e-4]


def get_data(prefix, short=False):
    output_data = []
    for d in link_delays:
        print d
        buffer_data = {}
        t = 'short_reconfig' if short else 'strobe'
        for fn in glob.glob(prefix + '/tmp/*-delay_sensitivity-%s-*'
                            'False-*-%.4f-*click.txt' % (t, d)):
            print fn
            buffer_size = int(
                fn.split('/')[-1].split('-QUEUE')[0].split('-')[-1])
            tput, lat, _, _, _, _, _ = get_tput_and_lat(fn)
            tput = tput[SR]
            lat50 = [x[1] for x in zip(*lat)[1]][0]
            buffer_data[buffer_size] = (tput, lat50)
        buffer_data = sorted(buffer_data.values())
        print buffer_data
        median_data = []
        for fn in glob.glob(prefix + '/tmp/*-delay_sensitivity-%s-*'
                            'True-*-%.4f-*click.txt' % (t, d)):
            print fn
            tput, lat, _, _, _, _, _ = get_tput_and_lat(fn)
            tput = tput[SR]
            lat50 = [x[1] for x in zip(*lat)[1]][0]
            print [tput]
            print zip(*buffer_data)[0]
            print zip(*buffer_data)[1]
            interp_latency = np.interp(
                [tput], zip(*buffer_data)[0], zip(*buffer_data)[1])[0]
            print lat50, interp_latency
            reduction = float(lat50) / interp_latency
            print reduction
            median_data.append((1-reduction)*100)
        print 'median_data', median_data
        output_data.append(np.median(median_data))
        print output_data[-1]
    return output_data

if __name__ == '__main__':
    if not os.path.isdir(sys.argv[1]):
        print 'first arg must be dir'
        sys.exit(-1)
    db = shelve.open(sys.argv[1] + '/delay_shelve.db')

    if 'output_data' in db:
        output_data = db['output_data']
    else:
        output_data = {}
        fn = sys.argv[1]
        output_data['strobe'] = get_data(fn, False)
        fn = fn.split('long')[0] + 'short'
        output_data['short_reconfig'] = get_data(fn, True)
        db['output_data'] = output_data
        
    # x = [[d / 2e-4 for d in link_delays] for i in output_data]
    x = [map(lambda x: x*1e6 / 20.0, link_delays) for i in output_data]

    y = [[100 - i for i in output_data['strobe']],
         [100 - i for i in output_data['short_reconfig']]]
    print 'delays:', x
    print 'latency:', y

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['180$\mu$s days', '90$\mu$s days']
    options.legend.options.fontsize = 19
    options.series_options = [DotMap(color='C%d' % i, marker='o', markersize=10, linewidth=5)
                              for i in range(len(x))]
    options.output_fn = 'graphs/delay_sensitivity.pdf'
    options.x.label.xlabel = 'Circuit link delay ($\mu$s)'
    options.y.label.ylabel = 'Median lat. (% of stat. buf.)'
    options.x.limits = [0, 130]
    options.y.limits = [50, 100]
    options.x.ticks.major.labels = DotMap(
        locations = [0, 10, 20, 30, 40, 60, 80, 120])

    plot(x, y, options)

    x = []
    x.append(map(lambda x: 180 / ((x*1e6 / 20.0) * 2), link_delays))
    x.append(map(lambda x: 90 / ((x*1e6 / 20.0) * 2), link_delays))

    y = [[i for i in output_data['strobe']],
         [i for i in output_data['short_reconfig']]]
    print 'delays:', x
    print 'latency:', y
    d0 = sorted(zip(x[0], y[0]))
    # d0 = d0[:-1]
    d1 = sorted(zip(x[1], y[1]))
    d1 = d1[:2] + d1[3:]
    x = [zip(*d0)[0], zip(*d1)[0]]
    y = [zip(*d0)[1], zip(*d1)[1]]
    x = [zip(*d0)[0]]
    y = [zip(*d0)[1]]

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['180$\mu$s days', '90$\mu$s days']
    options.legend.options.labels = []
    options.legend.options.fontsize = 19
    # options.legend.options.loc = 'upper right'
    options.series_options = [DotMap(color='C%d' % i, marker='o', markersize=10, linewidth=5)
                              for i in range(len(x))]
    options.output_fn = 'graphs/delay_sensitivity_rtt.pdf'
    options.x.label.xlabel = '# of RTTs in a day'
    options.y.label.ylabel = 'Median lat. improvement (%)'
    options.x.limits = [0, 19]
    # options.y.limits = [50, 100]
    options.y.limits = [0, 55]
    options.x.ticks.major.labels = DotMap(
        locations = [0, 1, 2, 3, 4, 5, 6, 9, 18])
    options.y.ticks.major.labels = DotMap(
        locations = [0, 10, 20, 30, 40, 50])

    plot(x, y, options)
    db.close()
