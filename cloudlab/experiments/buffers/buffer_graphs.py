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

experiment_names = {'rack_to_rack': '1505511786'}
buffer_sizes = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]

tputs = []
pings = []

for pattern, t in experiment_names.items():
    for b in buffer_sizes:
        tput, ping = get_tput_and_lat(pattern, t, str(b))
        tputs.append(tput)
        pings.append(ping)

tputs, pings = zip(*sorted(zip(tputs, pings)))

# throughput vs latency
x = [zip(*tputs)[0]]
y = [zip(*pings)[0]]
xerr = zip(*tputs)[1]
yerr = zip(*pings)[1]
print 'tput:', x
print 'latency:', y
print 'tput var:', xerr
print 'lat var:', yerr

options.series_options = [DotMap(color="C0", marker='o', linewidth=1,
                                 yerr=yerr)]
options.output_fn = 'throughput_vs_latency.pdf'
options.x.label.xlabel = 'Throughput (Gbps)'
options.y.label.ylabel = 'Latency (us)'

print options.output_fn
plot(x, y, options)


# size vs latency
x = [buffer_sizes]
y = [zip(*pings)[0]]
yerr = zip(*pings)[1]
print 'buffer:', x
print 'latency:', y
print 'lat var:', yerr

options.series_options = [DotMap(color="C0", marker='o', linewidth=1,
                                 yerr=yerr)]
options.output_fn = 'buffer_size_vs_latency.pdf'
options.x.label.xlabel = 'Buffer size (packets)'
options.y.label.ylabel = 'Latency (us)'

print options.output_fn
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
options.output_fn = 'bufer_size_vs_throughput.pdf'
options.x.label.xlabel = 'Buffer size (packets)'
options.y.label.ylabel = 'Throughput (Gbps)'

print options.output_fn
plot(x, y, options)
