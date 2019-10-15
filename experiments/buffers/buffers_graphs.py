#!/usr/bin/env python

import collections
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

import dotmap
from matplotlib import pyplot
import simpleplotlib
simpleplotlib.default_options.rcParams['font.family'] = "Tahoma"

import parse_logs
import python_config

FLW = (1, 2)

TYPES = ['static', 'resize', 'reTCP', 'reTCP+resize']

FILES = {
    'static': '*-strobe-*-False-*-cubic-*-400-3600-click.txt',
    'resize': '*-QUEUE-True-*-cubic-*-400-3600-click.txt',
    'reTCP': '*-QUEUE-False-*-retcp-*-400-3600-click.txt',
    'reTCP+resize': '*-QUEUE-True-*-retcp-*-400-3600-click.txt',
}

KEY_FNC = {
    'static': lambda fn: int(fn.split('strobe-')[1].split('-')[0]),
    'resize': lambda fn: (int(fn.split('True-')[1].split('-')[0])
                          / python_config.TDF),
    'reTCP': lambda fn: 0,
    'reTCP+resize': lambda fn: (int(fn.split('True-')[1].split('-')[0])
                                / python_config.TDF),
}


def get_data(db, key, files=FILES, key_fnc=KEY_FNC):
    if key not in db:
        ptn = path.join(sys.argv[1], files[key])
        fns = glob.glob(ptn)
        assert fns, "Found no files for pattern: {}".format(ptn)
        print("Found files for pattern: {}".format(ptn))
        for fn in fns:
            print("    {}".format(fn))

        data = collections.defaultdict(lambda: collections.defaultdict(dict))
        for fn in fns:
            lbl = key_fnc[key](fn.split('/')[-1])
            _, lat, _, c_tput, _, _, _ = parse_logs.parse_packet_log(fn)
            data['lat'][50][lbl] = [x[1] for x in zip(*lat)[1]]
            data['lat'][99][lbl] = [x[1] for x in zip(*lat)[3]]
            data['circ_tput'][lbl] = c_tput[FLW]

        data['keys'] = list(zip(*sorted(data['circ_tput'].items()))[0])
        data['lat'][50] = list(zip(*sorted(data['lat'][50].items()))[1])
        data['lat'][99] = list(zip(*sorted(data['lat'][99].items()))[1])
        data['circ_tput'] = list(zip(*sorted(data['circ_tput'].items()))[1])

        # Store the new data in the database.
        db[key] = dict(data)
    return db[key]


def graph_lat(keys, latencies, fn, ylb, odr=path.join(PROGDIR, "graphs")):
    # Sort the data based on the x-values (keys).
    keys, latencies = zip(
        *sorted(zip(keys, latencies), key=lambda p: int(p[0])))

    x = [keys for i in xrange(len(latencies[0]))]
    y = zip(*latencies)

    print("")
    print("raw latency data for: {}".format(fn))
    print("{}:".format(ylb.strip("\n")))
    print("    all: {}".format(", ".join(["({}: {})".format(a, b) for a, b in zip(x[0], y[0])])))
    print("    circuit: {}".format(". ".join(["({}: {})".format(a, b) for a, b in zip(x[1], y[1])])))
    print("    packet: {}".format(", ".join(["({}: {})".format(a, b) for a, b in zip(x[2], y[2])])))
    print("")

    options = dotmap.DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['all traffic', 'only circuit',
                                     'only packet']
    options.legend.options.fontsize = 20
    options.x.label.fontsize = options.y.label.fontsize = 20
    options.x.ticks.major.options.labelsize = \
        options.y.ticks.major.options.labelsize = 20
    options.series_options = [
        dotmap.DotMap(marker='o', markersize=10, linewidth=5)
        for i in range(len(x))]
    options.output_fn = path.join(odr, '{}.pdf'.format(fn))
    options.x.label.xlabel = 'Buffer size (packets)' if 'static' in fn \
                             else 'Early buffer resizing ($\mu$s)'
    options.y.label.ylabel = '{} latency ($\mu$s)'.format(ylb)
    options.x.ticks.major.labels = \
        dotmap.DotMap(locations=[4, 8, 16, 32, 64, 128]) \
        if 'static' in fn else \
        dotmap.DotMap(locations=[0, 25, 50, 75, 100, 125, 150, 175, 200, 225])
    options.y.ticks.major.labels = dotmap.DotMap(
        locations=[0, 50, 100, 150, 200, 250, 300, 350])
    options.y.limits = [0, 350]
    simpleplotlib.plot(x, y, options)


def graph_circuit_util(keys, tputs, fn, xlb, odr=path.join(PROGDIR, "graphs"),
                       srt=True, xlr=0, lbs=23, flt=lambda key: True):
    """ srt: sort, xlr: x label rotation (degrees), lbs: bar label size """
    if srt:
        # Sort the data based on the x-values (keys).
        keys, tputs = zip(*sorted(zip(keys, tputs), key=lambda p: int(p[0])))

    # Filter.
    keys, tputs = zip(
        *[(key, tput) for key, tput in zip(keys, tputs) if flt(key)])

    x = [np.arange(len(tputs))]
    # Convert circuit throughput into utilization.
    y = [[min(
        tput / (0.9 * 1. / (python_config.NUM_RACKS - 1)
                * python_config.CIRCUIT_BW_Gbps) * 100,
        100.0)
          for tput in tputs]]

    print("")
    print("raw util data for: {}".format(fn))
    print("{}:".format(xlb))
    print("    {}".format(", ".join(["({}: {})".format(a, b) for a, b in zip(keys, y[0])])))
    print("")

    options = dotmap.DotMap()
    options.plot_type = 'BAR'
    options.legend.options.fontsize = 20
    options.bar_labels.format_string = '%1.0f'
    options.bar_labels.options.fontsize = lbs
    options.output_fn = path.join(odr, "{}.pdf".format(fn))
    options.y.limits = (0, 100)
    options.x.label.fontsize = options.y.label.fontsize = 20
    options.x.label.xlabel = xlb
    options.y.label.ylabel = 'Average circuit\nutilization (%)'
    options.x.ticks.major.options.labelsize = \
        options.y.ticks.major.options.labelsize = 20
    options.x.ticks.major.labels = dotmap.DotMap(text=keys)
    options.x.ticks.major.labels.options.rotation = xlr
    options.x.ticks.major.labels.options.rotation_mode = "anchor"
    options.x.ticks.major.labels.options.horizontalalignment = \
        "center" if xlr == 0 else "right"
    options.y.ticks.major.show = True
    options.x.ticks.major.show = False
    simpleplotlib.plot(x, y, options)


def graph_util_vs_latency(tputs, latencies, fn):
    x = [map(
        lambda j: min(
            j / (0.9 * 1. / (python_config.NUM_RACKS - 1)
                 * python_config.CIRCUIT_BW_Gbps) * 100,
            100.0),
        u)
         for t in tputs]
    y = [zip(*l)[0] for l in latencies]

    options = dotmap.DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Static buffers (vary size)',
                                     'Dynamic buffers (vary $\\tau$)',
                                     'reTCP',
                                     'reTCP + dynamic buffers (vary $\\tau$)']
    options.legend.options.fontsize = 19
    options.series_options = [
        dotmap.DotMap(marker='o', markersize=10, linewidth=5)
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
    options.y.ticks.major.labels = \
        dotmap.DotMap(locations=[0, 200, 400, 600, 800, 1000]) \
        if '99' in fn else \
        dotmap.DotMap(locations=[0, 100, 200, 300, 400, 500, 600])

    simpleplotlib.plot(x, y, options)


def lat(name, edr, odr, ptn, key_fnc, prc, ylb):
    # Names are of the form "<number>_<details>_<specific options>". Experiments
    # where <details> are the same should be based on the same data. Therefore,
    # use <details> as the database key.
    print("Plotting: {}".format(name))
    basename = name.split("_")[1]
    db = shelve.open(path.join(edr, "{}.db".format(basename)))
    data = get_data(
        db, basename, files={basename: ptn}, key_fnc={basename: key_fnc})
    db.close()
    graph_lat(data['keys'], data['lat'][prc], name, ylb, odr)
    pyplot.close()


def util(name, edr, odr, ptn, key_fnc, xlb, srt=True, xlr=0, lbs=23,
         flt=lambda key: True):
    """
    srt: sort, xlr: x label rotation (degrees), lbs: bar label fontsize,
    flt: filter function that takes in a key
    """
    print("Plotting: {}".format(name))
    basename = name.split("_")[1]
    db = shelve.open(path.join(edr, "{}.db".format(basename)))
    data = get_data(
        db, basename, files={basename: ptn}, key_fnc={basename: key_fnc})
    db.close()
    graph_circuit_util(
        data['keys'], data['circ_tput'], name, xlb, odr, srt, xlr, lbs, flt)
    pyplot.close()


def main():
    if not path.isdir(sys.argv[1]):
        print 'first arg must be dir'
        sys.exit(-1)
    db = shelve.open(sys.argv[1] + '/buffer_shelve.db')

    typ = 'static'
    data = get_data(db, typ)
    graph_lat(keys=data['keys'], latencies=data['lat'][50], fn=typ,
              ylb="Median")
    graph_circuit_util(data['circ_tput'], typ)

    typ = 'resize'
    data = get_data(db, typ)
    graph_lat(keys=data['keys'], latencies=data['lat'][50],
              fn="{}-median".format(typ), ylb="Median")
    graph_lat(keys=data['keys'], latencies=data['lat'][99],
              fn="{}-99".format(typ), ylb="99th percentile\n")
    graph_circuit_util([db['static']['circ_tput'][2]] + data['circ_tput'], typ)

    typ = 'reTCP'
    get_data(db, typ)

    typ = 'reTCP+resize'
    get_data(db, typ)

    tputs = [db[t]['circ_tput'] for t in TYPES]
    lat50 = [db[t]['lat'][50] for t in TYPES]
    lat99 = [db[t]['lat'][99] for t in TYPES]
    graph_util_vs_latency(tputs, lat50, '50')
    graph_util_vs_latency(tputs, lat99, '99')

    db.close()


if __name__ == '__main__':
    main()
