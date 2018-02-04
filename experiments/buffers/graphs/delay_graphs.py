#!/usr/bin/env python

from dotmap import DotMap

import sys
sys.path.insert(0, '../../')
sys.path.insert(0, '/Users/mukerjee/Dropbox/Research/simpleplotlib/')

import shelve
import os
import glob
import numpy as np

from multiprocessing import Pool
from collections import defaultdict
from get_throughput_and_latency import get_tput_and_lat
from simpleplotlib import plot

DAY_LEN = 180
TDF = 20
SR = (1, 2)
link_delays = [1e-4, 2e-4, 3e-4, 4e-4, 6e-4, 8e-4, 12e-4, 16e-4, 24e-4]

types = ['static', 'resize']

files = {
    'static': '/tmp/*-strobe-*-False-*-%.4f-*click.txt',
    'resize': '/tmp/*-QUEUE-True-*-%.4f-*click.txt',
}

key_fn = {
    'static': lambda fn: int(fn.split('strobe-')[1].split('-')[0]),
    'resize': lambda fn: int(fn.split('True-')[1].split('-')[0]) / 20.0,
}


def get_data_from_file(fn):
    key = key_fn['static'](fn.split('/')[-1])
    print fn, key
    _, lat, _, circ_util, _, _, _ = get_tput_and_lat(fn)
    lat50 = [x[1] for x in zip(*lat)[1]][0]
    circ_util = circ_util[SR]
    return key, (circ_util, lat50)


def get_data():
    if 'data' in db:
        return db['data']
    data = []
    for d in link_delays:
        print d
        p = Pool()
        buffer_data = dict(p.map(get_data_from_file,
                                 glob.glob(sys.argv[1] + files['static'] % d)))
        p.close()
        buffer_data = sorted(buffer_data.values())
        fn = glob.glob(sys.argv[1] + files['resize'] % d)[0]
        print fn
        _, lat, _, circ_util, _, _, _ = get_tput_and_lat(fn)
        lat50 = [x[1] for x in zip(*lat)[1]][0]
        circ_util = circ_util[SR]
        interp_latency = np.interp(
            [circ_util], zip(*buffer_data)[0], zip(*buffer_data)[1])[0]
        reduction = (1 - (float(lat50) / interp_latency)) * 100
        num_of_rtts = DAY_LEN / (d * 1e6 / TDF * 2)
        data.append((num_of_rtts, reduction))
        print 'reduction', reduction
    return sorted(data)

if __name__ == '__main__':
    if not os.path.isdir(sys.argv[1]):
        print 'first arg must be dir'
        sys.exit(-1)
    db = shelve.open(sys.argv[1] + '/delay_shelve.db')
    db['data'] = get_data()

    x = [zip(*db['data'])[0]]
    y = [zip(*db['data'])[1]]

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.fontsize = 19
    options.series_options = [DotMap(marker='o', markersize=10, linewidth=5)
                              for i in range(len(x))]
    options.output_fn = 'graphs/delay_sensitivity_rtt.pdf'
    options.x.label.xlabel = '# of RTTs in a day'
    options.y.label.ylabel = 'Median lat. improve. (%)'
    options.x.limits = [0, 19]
    options.y.limits = [0, 55]
    options.x.ticks.major.labels = DotMap(
        locations=[0, 1, 2, 3, 4, 5, 6, 9, 18])
    options.y.ticks.major.labels = DotMap(
        locations=[0, 10, 20, 30, 40, 50])

    plot(x, y, options)
    db.close()
