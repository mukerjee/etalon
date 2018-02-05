#!/usr/bin/env PYTHONPATH=../ python

import sys
import os
import copy
import shelve
import glob

from multiprocessing import Pool
from collections import defaultdict
from dotmap import DotMap
from simpleplotlib import plot
from parse_logs import get_seq_data

types = ['static', 'resize']

files = {
    'static': '/tmp/*-strobe-*-False-*-reno-*click.txt',
    'resize': '/tmp/*-QUEUE-True-*-reno-*click.txt',
}

key_fn = {
    'static': lambda fn: int(fn.split('strobe-')[1].split('-')[0]),
    'resize': lambda fn: int(fn.split('True-')[1].split('-')[0]) / 20.0,
}

units = 1000.0  # Kilo-sequence number
num_hosts = 16.0
time_length = 4200


class FileReader(object):
    def __init__(self, name):
        self.name = name

    def __call__(self, fn):
        key = key_fn[self.name](fn.split('/')[-1])
        print fn, key
        return int(round(float(key))), get_seq_data(fn)


def get_data(name):
    if name in db:
        return db[name]
    else:
        data = defaultdict(dict)
        p = Pool()
        data['raw_data'] = dict(p.map(FileReader(name),
                                      glob.glob(sys.argv[1] + files[name])))
        data['raw_data'] = sorted(data['raw_data'].items())
        data['keys'] = list(zip(*data['raw_data'])[0]) + ['Optimal']
        data['lines'] = data['raw_data'][0][1][1]
        data['data'] = [map(lambda x: x / units, f) for f in
                        zip(*zip(*data['raw_data'])[1])[0]]

        c_start, c_end, nc_start, nc_end, \
            nnc_start, nnc_end = [int(round(q)) for q in data['lines']]
        packet_rate = 10*10**9 / 8.0 / units * 1e-06 / num_hosts
        circuit_rate = 80*10**9 / 8.0 / units * 1e-06 / num_hosts
        optimal = [packet_rate * i for i in xrange(c_start)]
        optimal += [circuit_rate * i + optimal[-1]
                    for i in xrange(c_end - c_start)]
        optimal += [packet_rate * i + optimal[-1]
                    for i in xrange(nc_start - c_end)]
        optimal += [circuit_rate * i + optimal[-1]
                    for i in xrange(nc_end - nc_start)]
        optimal += [packet_rate * i + optimal[-1]
                    for i in xrange(nnc_start - nc_end)]
        optimal += [circuit_rate * i + optimal[-1]
                    for i in xrange(time_length - nnc_start)]
        data['data'].append(optimal)
        return dict(data)


def plot_seq(data, fn):
    x = [xrange(time_length) for i in xrange(len(data['keys']))]
    y = data['data']

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = \
        ['%s packets' % k for k in data['keys'][:-1]] if 'static' in fn \
        else ['%s $\mu$s' % k for k in data['keys'][:-1]]
    options.legend.options.labels += ['Optimal']
    options.legend.options.fontsize = 16
    options.legend.options.ncol = 2
    options.series_options = [DotMap(linewidth=2) for i in range(len(x))]
    options.output_fn = 'graphs/seq_%s.pdf' % fn
    options.x.label.xlabel = 'Time (us)'
    options.y.label.ylabel = 'Expected seq. num. (K)'
    options.vertical_lines.lines = data['lines']
    lines = data['lines']
    shaded = []
    for i in xrange(0, len(lines), 2):
        shaded.append((lines[i], lines[i+1]))
    options.vertical_shaded.limits = shaded
    options.vertical_shaded.options.alpha = 0.1
    options.vertical_shaded.options.color = 'blue'

    if 'resize' in fn:
        options.inset.show = True
        options.inset.options.zoom_level = 3
        options.inset.options.corners = [2, 3]
        options.inset.options.marker.options.color = 'black'
        options.inset.options.x.limits = [2600, 2830]
        options.inset.options.y.limits = [220, 430]

    plot(x, y, options)

    
if __name__ == '__main__':
    if not os.path.isdir(sys.argv[1]):
        print 'first arg must be dir'
        sys.exit(-1)
    db = shelve.open(sys.argv[1] + '/seq_shelve.db')
    db['static'] = get_data('static')
    plot_seq(db['static'], 'static')

    db['resize'] = get_data('resize')
    resize_data = copy.copy(db['resize'])
    resize_data['lines'] = db['static']['lines']
    resize_data['keys'] = [0] + db['resize']['keys']
    resize_data['data'] = [db['static']['data'][2]] + db['resize']['data']
    plot_seq(resize_data, 'resize')

    db.close()
