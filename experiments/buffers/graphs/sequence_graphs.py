#!/usr/bin/env python

from dotmap import DotMap

import sys
sys.path.insert(0, '../../')
sys.path.insert(0, '/Users/mukerjee/Dropbox/Research/simpleplotlib/')

import glob
import numpy as np
import matplotlib.pyplot as plt

from get_throughput_and_latency import get_tput_and_lat, get_seq_data
from simpleplotlib import plot

if __name__ == '__main__':
    circuit_data = {}
    for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-circuit-*-'
                        'QUEUE-False-*-reno-*click.txt'):
        buffer_size = int(fn.split('/')[-1].split('circuit-')[1].split('-')[0])
        print fn
        circuit_data[buffer_size] = get_seq_data(fn)
    circuit_data = sorted(circuit_data.items())
    
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
    plot(x, y, options)
    
    packet_data = {}
    for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-no_circuit-*-'
                        'QUEUE-False-*-reno-*click.txt'):
        buffer_size = int(fn.split('/')[-1].split('no_circuit-')[1].split(
            '-')[0])
        print fn
        packet_data[buffer_size] = get_seq_data(fn)
    packet_data = sorted(packet_data.items())
    
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
    plot(x, y, options)
    
    buffer_data = {}
    for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-*-'
                        'QUEUE-False-*-reno-*click.txt'):
        buffer_size = int(fn.split('/')[-1].split('strobe-')[1].split('-')[0])
        print fn
        buffer_data[buffer_size] = get_seq_data(fn)
    buffer_data = sorted(buffer_data.items())

    x = [xrange(4000) for i in xrange(len(buffer_data))]
    y = [b[1][0] for b in buffer_data]
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = [str(b[0]) for b in buffer_data]
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, linewidth=1)
                              for i in range(len(x))]
    options.output_fn = 'graphs/seq_buffer.pdf'
    options.x.label.xlabel = 'Time (us)'
    options.y.label.ylabel = 'Sequence Number'
    options.vertical_lines.lines = buffer_data[0][1][1]
    plot(x, y, options)

    days_out_data = {0: buffer_data[2][1]}
    for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-16-'
                        'QUEUE-True-*-reno-*click.txt'):
        days_out = int(fn.split('/')[-1].split('True-')[1].split('-')[0])
        print fn
        days_out_data[days_out] = get_seq_data(fn)
    days_out_data = sorted(days_out_data.items())

    x = [xrange(4000) for i in xrange(len(days_out_data))]
    y = [b[1][0] for b in days_out_data]
    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = [str(b[0]) for b in days_out_data]
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, linewidth=1)
                              for i in range(len(x))]
    options.output_fn = 'graphs/seq_days_out.pdf'
    options.x.label.xlabel = 'Time (us)'
    options.y.label.ylabel = 'Sequence Number'
    options.vertical_lines.lines = days_out_data[0][1][1]
    plot(x, y, options)

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
    plot(x, y, options)
