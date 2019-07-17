#!/usr/bin/env python

from collections import defaultdict
import copy
import glob
from multiprocessing import Pool
import os
from os import path
import shelve
import sys
# For parse_logs.
PROGDIR = path.dirname(path.realpath(__file__))
sys.path.insert(0, path.join(PROGDIR, '..'))

from dotmap import DotMap
from simpleplotlib import plot

from parse_logs import get_seq_data

# Maps experiment to filename.
files = {
    'static': '/*-strobe-*-False-*-reno-*click.txt',
    'resize': '/*-QUEUE-True-*-reno-*click.txt',
}

# Maps experiment to a function that convert a filename to an integer key.
key_fn = {
    'static': lambda fn: int(fn.split('strobe-')[1].split('-')[0]),
    'resize': lambda fn: int(fn.split('True-')[1].split('-')[0]) / 20.0,
}

units = 1000.0  # Kilo-sequence number
num_hosts = 16.0
time_length = 1200


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
        # data["raw_data"] = A list of pairs, where each pair corresponds to an
        #   experiment file.
        # data["raw_data"][i] = A pair of (key value, results).
        # data["raw_data"][i][1] = A pair of (list, n-tuple).
        # data["raw_data"][i][1][0] = A list of expected sequence number over
        #   time.
        # data["raw_data"][i][1][1] = An n-tuple of the times of circuit up/down
        #   events.
        # data["raw_data"][i][1][1][0] = The time at which the first day began.

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
    options.legend.options.fontsize = 14
    options.legend.options.ncol = 2
    options.series_options = [DotMap(linewidth=2) for i in range(len(x))]
    options.output_fn = path.join(PROGDIR, 'graphs', 'seq_%s.pdf' % fn)
    options.x.label.xlabel = 'Time (us)'
    options.y.label.ylabel = 'Expected seq. num. (K)'
    options.x.label.fontsize = options.y.label.fontsize = 16
    options.x.ticks.major.options.labelsize = \
        options.y.ticks.major.options.labelsize = 16
    options.x.axis.show = options.y.axis.show = True
    options.x.axis.color = options.y.axis.color = 'black'
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
        options.inset.options.zoom_level = 2
        options.inset.options.corners = [2, 3]
        options.inset.options.marker.options.color = 'black'
        options.inset.options.x.limits = [620, 800]
        options.inset.options.y.limits = [70, 270]

    # Hack to pick only the lines that we want.
    iss = [0, 2, 3, 4, 5, 6, 7, 8, 9, 17]
    # iss = [0, 9, 10, 11, 12, 13, 14, 15, 16, 17]
    x, y, options.legend.options.labels = zip(
        *[(a, b, l) for (i, (a, b, l)) in enumerate(
            zip(x, y, options.legend.options.labels))
          if i in iss])

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
