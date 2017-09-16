#!/usr/bin/env python

from dotmap import DotMap

import sys
sys.path.append('../')
sys.path.insert(0, '/Users/mukerjee/Dropbox/Research/simpleplotlib/')

from graph_common import get_tput_and_lat
from simpleplotlib import plot

options = DotMap()
options.plot_type = 'LINE'

options.legend.options.labels = ['Rack to Rack']
options.legend.options.fontsize = 17

experiment_names = {'one_to_one': sys.argv[1]}
config_types = ['normal', 'no_circuit', 'strobe', 'resize']
buffer_sizes = [1, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50]

default_config = {'type': 'normal', 'buffer_size': 40,
                  'traffic_source': 'QUEUE', 'queue_resize': False}


for ct in config_types:
    config = dict(default_config)
    config['type'] = ct
    if ct == 'resize':
        config['type'] = 'normal'
        config['queue_resize'] = True

    tputs = []
    pings = []

    for pattern, t in experiment_names.items():
        for b in buffer_sizes:
            config['buffer_size'] = b
            tput, ping = get_tput_and_lat(pattern, t, config)
            tputs.append(tput)
            pings.append(ping)


    # size vs latency
    x = [buffer_sizes]
    y = [zip(*pings)[0]]
    yerr = zip(*pings)[1]
    print 'buffer:', x
    print 'latency:', y
    print 'lat var:', yerr

    options.series_options = [DotMap(color="C0", marker='o', linewidth=1,
                                     yerr=yerr)]
    options.output_fn = 'graphs/%s-buffer_size_vs_latency.pdf' % ct
    options.x.label.xlabel = 'Buffer size (packets)'
    options.y.label.ylabel = 'Latency (us)'
    plot(x, y, options)


    # size vs throughput
    x = [buffer_sizes]
    y = [zip(*tputs)[0]]
    yerr = zip(*tputs)[1]
    print 'buffer:', x
    print 'tput:', y
    print 'tput var:', yerr

    options.series_options = [DotMap(color="C0", marker='o', linewidth=1,
                                     yerr=yerr)]
    options.output_fn = 'graphs/%s-bufer_size_vs_throughput.pdf' % ct
    options.x.label.xlabel = 'Buffer size (packets)'
    options.y.label.ylabel = 'Throughput (Gbps)'

    plot(x, y, options)
    tputs_sorted, pings_sorted = zip(*sorted(zip(tputs, pings)))


    # throughput vs latency
    x = [zip(*tputs)[0]]
    y = [zip(*pings)[0]]
    xerr = zip(*tputs)[1]
    yerr = zip(*pings)[1]
    print 'tput:', x
    print 'latency:', y
    print 'tput var:', xerr
    print 'lat var:', yerr

    options.plot_type = 'SCATTER'
    options.series_options = [DotMap(color="C0", marker='o', linewidth=1)]
    options.output_fn = 'graphs/%s-throughput_vs_latency.pdf' % ct
    options.x.label.xlabel = 'Throughput (Gbps)'
    options.y.label.ylabel = 'Latency (us)'

    plot(x, y, options)
