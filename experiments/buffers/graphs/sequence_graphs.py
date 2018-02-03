#!/usr/bin/env python

from dotmap import DotMap

import sys
sys.path.insert(0, '../../')
sys.path.insert(0, '/Users/mukerjee/Dropbox/Research/simpleplotlib/')

import shelve
import os
import glob
import numpy as np
import matplotlib.pyplot as plt

from get_throughput_and_latency import get_tput_and_lat, get_seq_data
from simpleplotlib import plot

if __name__ == '__main__':
    if not os.path.isdir(sys.argv[1]):
        print 'first arg must be dir'
        sys.exit(-1)
    db = shelve.open(sys.argv[1] + '/seq_shelve.db')

    if 'circuit_data' in db:
        circuit_data = db['circuit_data']
    else:
        circuit_data = {}
        for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-circuit-*-'
                            'QUEUE-False-*-reno-*click.txt'):
            buffer_size = int(fn.split('/')[-1].split('circuit-')[1].split('-')[0])
            print fn
            circuit_data[buffer_size] = get_seq_data(fn)
        circuit_data = sorted(circuit_data.items())
        db['circuit_data'] = circuit_data
    
    x = [xrange(4000) for i in xrange(len(circuit_data))]
    y = [b[1][0] for b in circuit_data]
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = [str(b[0]) for b in circuit_data]
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, linewidth=1)
                              for i in range(len(x))]
    options.output_fn = 'graphs/seq_circuit.pdf'
    options.x.label.xlabel = 'Time (us)'
    options.y.label.ylabel = 'Sequence Number'
    options.vertical_lines.lines = circuit_data[0][1][1]
    # plot(x, y, options)

    if 'packet_data' in db:
        packet_data = db['packet_data']
    else:
        packet_data = {}
        for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-no_circuit-*-'
                            'QUEUE-False-*-reno-*click.txt'):
            buffer_size = int(fn.split('/')[-1].split('no_circuit-')[1].split(
                '-')[0])
            print fn
            packet_data[buffer_size] = get_seq_data(fn)
        packet_data = sorted(packet_data.items())
        db['packet_data'] = packet_data
    
    x = [xrange(4000) for i in xrange(len(packet_data))]
    y = [b[1][0] for b in packet_data]
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = [str(b[0]) for b in packet_data]
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, linewidth=1)
                              for i in range(len(x))]
    options.output_fn = 'graphs/seq_packet.pdf'
    options.x.label.xlabel = 'Time (us)'
    options.y.label.ylabel = 'Sequence Number'
    options.vertical_lines.lines = packet_data[0][1][1]
    # plot(x, y, options)
    
    if 'buffer_data' in db:
        buffer_data = db['buffer_data']
    else:
        buffer_data = {}
        for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-*-'
                            'QUEUE-False-*-reno-*click.txt'):
            buffer_size = int(fn.split('/')[-1].split('strobe-')[1].split('-')[0])
            print fn
            buffer_data[buffer_size] = get_seq_data(fn)
        buffer_data = sorted(buffer_data.items())
        for i in xrange(len(buffer_data)):
            buffer_data[i] = (str(buffer_data[i][0]) + ' packets',
                                buffer_data[i][1])
        db['buffer_data'] = buffer_data

    c_start, c_end, nc_start, nc_end, \
        nnc_start, nnc_end = [int(round(q))
                              for q in buffer_data[0][1][1]]
    x = [xrange(4200) for i in xrange(len(buffer_data)+1)]

    units = 1000.0  # 1024.0
    optimal = [10*10**9 / 8.0 / units * 1e-06 / 16.0 * i
               for i in xrange(c_start)]
    endpoint = optimal[-1]
    optimal += [80*10**9 / 8.0 / units * 1e-06 / 16.0 * i + endpoint
                for i in xrange(c_end - c_start)]
    endpoint = optimal[-1]
    optimal += [10*10**9 / 8.0 / units * 1e-06 / 16.0 * i + endpoint
                for i in xrange(nc_start - c_end)]
    endpoint = optimal[-1]
    optimal += [80*10**9 / 8.0 / units * 1e-06 / 16.0 * i + endpoint
                for i in xrange(nc_end - nc_start)]
    endpoint = optimal[-1]
    optimal += [10*10**9 / 8.0 / units * 1e-06 / 16.0 * i + endpoint
                for i in xrange(nnc_start - nc_end)]
    endpoint = optimal[-1]
    optimal += [80*10**9 / 8.0 / units * 1e-06 / 16.0 * i + endpoint
                for i in xrange(4200 - nnc_start)]
        
    y = [[q / 1000.0 for q in b[1][0]] for b in buffer_data] + [optimal]
    # y = [b[1][0] for b in buffer_data] + [optimal]

    print len(x[0]), len(x[1]), len(y[0]), len(y[1])
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = [str(b[0]) for b in buffer_data] + \
                                    ["Optimal"]
    options.legend.options.fontsize = 16
    options.legend.options.ncol = 2
    options.series_options = [DotMap(color='C%d' % i, linewidth=2)
                              for i in range(len(x))]
    options.output_fn = 'graphs/seq_buffer.pdf'
    options.x.label.xlabel = 'Time (us)'
    options.y.label.ylabel = 'Expected seq. num. (K)'
    options.vertical_lines.lines = buffer_data[0][1][1]
    lines = buffer_data[0][1][1]
    shaded = []
    for i in xrange(0, len(lines), 2):
        shaded.append((lines[i], lines[i+1]))
    options.vertical_shaded.limits = shaded
    options.vertical_shaded.options.alpha = 0.1
    options.vertical_shaded.options.color = 'blue'
    plot(x, y, options)

    if 'days_out_data' in db:
        days_out_data = db['days_out_data']
    else:
        days_out_data = {0: buffer_data[2][1]}
        for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-16-'
                            'QUEUE-True-*-reno-*click.txt'):
            days_out = int(fn.split('/')[-1].split('True-')[1].split('-')[0]) / 20
            print fn
            days_out_data[days_out] = get_seq_data(fn)
        days_out_data = sorted(days_out_data.items())
        for i in xrange(len(days_out_data)):
            days_out_data[i] = (str(days_out_data[i][0]) + ' $\mu$s',
                                days_out_data[i][1])
        db['days_out_data'] = days_out_data

    c_start, c_end, nc_start, nc_end, \
        nnc_start, nnc_end = [int(round(q))
                              for q in days_out_data[0][1][1]]
    x = [xrange(4200) for i in xrange(len(days_out_data)+1)]

    units = 1000.0  # 1024.0
    optimal = [10*10**9 / 8.0 / units * 1e-06 / 16.0 * i
               for i in xrange(c_start)]
    endpoint = optimal[-1]
    optimal += [80*10**9 / 8.0 / units * 1e-06 / 16.0 * i + endpoint
                for i in xrange(c_end - c_start)]
    endpoint = optimal[-1]
    optimal += [10*10**9 / 8.0 / units * 1e-06 / 16.0 * i + endpoint
                for i in xrange(nc_start - c_end)]
    endpoint = optimal[-1]
    optimal += [80*10**9 / 8.0 / units * 1e-06 / 16.0 * i + endpoint
                for i in xrange(nc_end - nc_start)]
    endpoint = optimal[-1]
    optimal += [10*10**9 / 8.0 / units * 1e-06 / 16.0 * i + endpoint
                for i in xrange(nnc_start - nc_end)]
    endpoint = optimal[-1]
    optimal += [80*10**9 / 8.0 / units * 1e-06 / 16.0 * i + endpoint
                for i in xrange(4200 - nnc_start)]
        
    y = [[q / 1000.0 for q in b[1][0]] for b in days_out_data] + [optimal]
    # y = [b[1][0] for b in days_out_data] + [optimal]

    print len(x[0]), len(x[1]), len(y[0]), len(y[1])
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = [str(b[0]) for b in days_out_data] + \
                                    ["Optimal"]
    options.legend.options.fontsize = 16
    options.legend.options.ncol = 2
    options.series_options = [DotMap(color='C%d' % i, linewidth=2)
                              for i in range(len(x))]
    options.output_fn = 'graphs/seq_days_out.pdf'
    options.x.label.xlabel = 'Time (us)'
    options.y.label.ylabel = 'Expected seq. num. (K)'
    options.vertical_lines.lines = days_out_data[0][1][1]
    lines = days_out_data[0][1][1]
    shaded = []
    for i in xrange(0, len(lines), 2):
        shaded.append((lines[i], lines[i+1]))
    options.vertical_shaded.limits = shaded
    options.vertical_shaded.options.alpha = 0.1
    options.vertical_shaded.options.color = 'blue'

    options.inset.show = True
    options.inset.options.zoom_level = 3
    options.inset.options.corners = [2, 3]
    options.inset.options.marker.options.color = 'black'
    options.inset.options.x.limits = [2600, 2830]
    options.inset.options.y.limits = [220, 430]
    plot(x, y, options)

    if 'ocs_days_out_data' in db:
        ocs_days_out_data = db['ocs_days_out_data']
    else:
        for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-16-'
                            'QUEUE-False-*-ocs-*click.txt'):
            print fn
            ocs_strobe_data = get_seq_data(fn)

        ocs_days_out_data = {0: ocs_strobe_data}
        for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-16-'
                            'QUEUE-True-*-ocs-*click.txt'):
            days_out = int(fn.split('/')[-1].split('True-')[1].split('-')[0])
            print fn
            ocs_days_out_data[days_out] = get_seq_data(fn)
        ocs_days_out_data = sorted(ocs_days_out_data.items())
        db['ocs_days_out_data'] = ocs_days_out_data

    x = [xrange(4000) for i in xrange(len(ocs_days_out_data))]
    y = [b[1][0] for b in ocs_days_out_data]
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = [str(b[0]) for b in ocs_days_out_data]
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, linewidth=1)
                              for i in range(len(x))]
    options.output_fn = 'graphs/seq_ocs.pdf'
    options.x.label.xlabel = 'Time (us)'
    options.y.label.ylabel = 'Sequence Number'
    options.vertical_lines.lines = ocs_days_out_data[0][1][1]
    # plot(x, y, options)

    db.close()
