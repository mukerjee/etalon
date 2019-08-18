#!/usr/bin/env python

from os import path
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For parse_logs.
sys.path.insert(0, path.join(PROGDIR, ".."))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "..", "etc"))
import shelve
import glob
import numpy as np

from collections import defaultdict
from dotmap import DotMap
from simpleplotlib import plot

import parse_logs as pl
import python_config as pyc


SR = (1, 2)

TYPES = ['static', 'resize', 'reTCP', 'reTCP+resize']

FILES = {
    'static': '*-strobe-*-False-*-cubic-*-400-3600-click.txt',
    'resize': '*-QUEUE-True-*-cubic-*-400-3600-click.txt',
    'reTCP': '*-QUEUE-False-*-retcp-*-400-3600-click.txt',
    'reTCP+resize': '*-QUEUE-True-*-retcp-*-400-3600-click.txt',
}

KEY_FN = {
    'static': lambda fn: int(fn.split('strobe-')[1].split('-')[0]),
    'resize': lambda fn: int(fn.split('True-')[1].split('-')[0]) / pyc.TDF,
    'reTCP': lambda fn: 0,
    'reTCP+resize': lambda fn: int(fn.split('True-')[1].split('-')[0]) / pyc.TDF,
}


def get_data(name, files=FILES, key_fn=KEY_FN):
    if name in db:
        return db[name]
    else:
        data = defaultdict(lambda: defaultdict(dict))

        ptn = path.join(sys.argv[1], files[name])
        fns = glob.glob(ptn)
        assert len(fns) > 0, "Found no files for pattern: {}".format(ptn)
        print("Found files for pattern: {}".format(ptn))
        for fn in fns:
            print("    {}".format(fn))

        for fn in fns:
            key = key_fn[name](fn.split('/')[-1])
            print("key: {}".format(key))
            _, lat, _, circ_util, _, _, _ = pl.parse_packet_log(fn)
            data['lat'][50][key] = [x[1] for x in zip(*lat)[1]]
            data['lat'][99][key] = [x[1] for x in zip(*lat)[3]]
            data['circ_util'][key] = circ_util[SR]
        data['keys'] = list(zip(*sorted(data['circ_util'].items()))[0])
        data['lat'][50] = list(zip(*sorted(data['lat'][50].items()))[1])
        data['lat'][99] = list(zip(*sorted(data['lat'][99].items()))[1])
        data['circ_util'] = list(zip(*sorted(data['circ_util'].items()))[1])
        return dict(data)


def graph_lat(keys, latencies, fn, y_lab):
    x = [keys for i in xrange(len(latencies[0]))]
    y = zip(*latencies)

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['All traffic', 'Only circuit',
                                     'Only packet']
    options.legend.options.fontsize = 19
    options.series_options = [DotMap(marker='o', markersize=10, linewidth=5)
                              for i in range(len(x))]
    options.output_fn = path.join(PROGDIR, 'graphs', '{}_vs_latency.pdf'.format(fn))
    options.x.label.xlabel = 'Buffer size (packets)' if 'static' in fn \
                             else 'Early buffer resizing ($\mu$s)'
    options.y.label.ylabel = '{} latency ($\mu$s)'.format(y_lab)
    options.x.ticks.major.labels = DotMap(
        locations=[4, 8, 16, 32, 64, 128]) if 'static' in fn \
        else DotMap(locations=[0, 25, 50, 75, 100, 125, 150, 175, 200, 225])
    options.y.ticks.major.labels = DotMap(
        locations=[0, 50, 100, 150, 200, 250, 300, 350])
    options.y.limits = [0, 350]
    plot(x, y, options)


def graph_circuit_util(util, fn):
    x = [np.arange(len(util))]
    y = [map(lambda j: min(j / (0.9 * 1.0/7 * 80) * 100, 100.0), util)]

    options = DotMap()
    options.plot_type = 'BAR'
    options.legend.options.fontsize = 12
    options.bar_labels.format_string = '%1.0f'
    options.bar_labels.options.fontsize = 25
    options.output_fn = path.join(
        PROGDIR, 'graphs', '{}_vs_circuit_util.pdf'.format(fn))
    options.x.label.xlabel = 'Buffer size (packets)' if 'static' in fn \
                             else 'Early buffer resizing ($\mu$s)'
    options.y.label.ylabel = 'Avg. circuit utilization (%)'
    options.x.ticks.major.labels = DotMap(
        text=[4, 8, 16, 32, 64, 128]) if 'static' in fn \
        else DotMap(text=[0, 200, 400, 600, 800, 1000, 1200, 1400])
    options.y.ticks.major.show = False
    options.x.ticks.major.show = False
    plot(x, y, options)


def graph_util_vs_latency(utils, latencies, fn):
    x = [map(lambda j: min(j / (0.9 * 1.0/7 * 80) * 100, 100.0), u)
         for u in utils]
    y = [zip(*l)[0] for l in latencies]

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Static buffers (vary size)',
                                     'Dynamic buffers (vary $\\tau$)',
                                     'reTCP',
                                     'reTCP + dynamic buffers (vary $\\tau$)']
    options.legend.options.fontsize = 19
    options.series_options = [DotMap(marker='o', markersize=10, linewidth=5)
                              for i in range(len(x))]
    options.series_options[2].marker = 'x'
    options.series_options[2].s = 100
    del options.series_options[2].markersize
    options.series_options[2].zorder = 10
    options.output_fn = \
        path.join(PROGDIR, 'graphs', 'throughput_vs_latency99.pdf') \
        if '99' in fn \
           else path.join(PROGDIR, 'graphs', 'throughput_vs_latency.pdf')
    options.x.label.xlabel = 'Circuit utilization (%)'
    options.y.label.ylabel = '99th percent. latency ($\mu$s)' if '99' in fn \
                             else 'Median latency ($\mu$s)'
    options.y.limits = [0, 1000] if '99' in fn else [0, 600]
    options.y.ticks.major.labels = DotMap(
        locations=[0, 200, 400, 600, 800, 1000]) if '99' in fn else \
        DotMap(locations=[0, 100, 200, 300, 400, 500, 600])

    plot(x, y, options)


if __name__ == '__main__':
    if not path.isdir(sys.argv[1]):
        print 'first arg must be dir'
        sys.exit(-1)
    db = shelve.open(sys.argv[1] + '/buffer_shelve.db')

    typ = 'static'
    db[typ] = get_data(typ)
    graph_lat(keys=db[typ]['keys'], latencies=db[typ]['lat'][50], fn=typ,
              y_lab="Median")
    graph_circuit_util(db[typ]['circ_util'], typ)

    typ = 'resize'
    db[typ] = get_data(typ)
    graph_lat(keys=[0] + db[typ]['keys'],
              latencies=[db['static']['lat'][50][2]] + db[typ]['lat'][50],
              fn="{}-median".format(typ),
              y_lab="Median")
    graph_lat(keys=[0] + db[typ]['keys'],
              latencies=[db['static']['lat'][99][2]] + db[typ]['lat'][99],
              fn="{}-99".format(typ), y_lab="99th percentile\n")
    graph_circuit_util([db['static']['circ_util'][2]] + db[typ]['circ_util'],
                       typ)

    typ = 'reTCP'
    db[typ] = get_data(typ)

    typ = 'reTCP+resize'
    db[typ] = get_data(typ)

    utils = [db[t]['circ_util'] for t in TYPES]
    lat50 = [db[t]['lat'][50] for t in TYPES]
    lat99 = [db[t]['lat'][99] for t in TYPES]
    graph_util_vs_latency(utils, lat50, '50')
    graph_util_vs_latency(utils, lat99, '99')

    db.close()
