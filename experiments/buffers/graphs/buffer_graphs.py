#!/usr/bin/env python

from dotmap import DotMap

import sys
sys.path.insert(0, '../../')
sys.path.insert(0, '/Users/mukerjee/Dropbox/Research/simpleplotlib/')

import shelve
import os
import glob
import numpy as np

from get_throughput_and_latency import get_tput_and_lat
from simpleplotlib import plot

SR = (1, 2)


def graph(data, x_label, fn, rtt=True, circuit=False, packet=False):
    graph_lat50(data, x_label, fn, circuit, packet)
    # graph_lat99(data, x_label, fn, circuit, packet)
    # graph_tput(data, x_label, fn)
    # if not circuit:
    #     graph_packet_util(data, x_label, fn, packet)
    if not packet:
        graph_circuit_util(data, x_label, fn, circuit)
    # if rtt:
    #     graph_rtts(data, x_label, fn)


def graph_lat50(data, x_label, fn, circuit, packet):
    x = [zip(*data)[0]]
    if not circuit and not packet:
        x = [zip(*data)[0], zip(*data)[0], zip(*data)[0]]
        y = zip(*zip(*data)[1])[1]
        y = zip(*y)
    if circuit:
        y = zip(*zip(*data)[1])[1]
        y = [zip(*y)[1]]
    if packet:
        y = zip(*zip(*data)[1])[1]
        y = [zip(*y)[2]]
    print 'buffer:', x
    print 'latency:', y

    options = DotMap()
    options.plot_type = 'LINE'
    if not circuit and not packet:
        options.legend.options.labels = ['All traffic', 'Only circuit',
                                         'Only packet']
    if circuit:
        options.legend.options.labels = ['Circuit']
    if packet:
        options.legend.options.labels = ['Packet']
    if fn == 'buffer_size':
        options.x.ticks.major.labels = DotMap(
            locations=[4, 8, 16, 32, 64, 128])
    if fn == 'days_out' or fn == 'ocs_days_out':
        options.x.ticks.major.labels = DotMap(
            locations=[0, 200, 400, 600, 800, 1000, 1200, 1400])
    options.y.ticks.major.labels = DotMap(
        locations=[0, 100, 200, 300, 400, 500, 600])
    options.legend.options.fontsize = 19
    options.series_options = [DotMap(color='C%d' % i, marker='o', markersize=10, linewidth=5)
                              for i in range(len(x))]
    options.output_fn = 'graphs/%s_vs_latency.pdf' % fn
    options.x.label.xlabel = x_label
    options.y.label.ylabel = 'Median latency ($\mu$s)'
    options.y.limits = [0, 600]
    plot(x, y, options)


def graph_lat99(data, x_label, fn, circuit, packet):
    x = [zip(*data)[0]]
    if not circuit and not packet:
        x = [zip(*data)[0], zip(*data)[0], zip(*data)[0]]
        y = zip(*zip(*data)[1])[2]
        y = zip(*y)
    if circuit:
        y = zip(*zip(*data)[1])[2]
        y = [zip(*y)[1]]
    if packet:
        y = zip(*zip(*data)[1])[2]
        y = [zip(*y)[2]]
    print 'buffer:', x
    print 'latency:', y

    options = DotMap()
    options.plot_type = 'LINE'
    if not circuit and not packet:
        options.legend.options.labels = ['Total', 'Circuit', 'Packet']
    if circuit:
        options.legend.options.labels = ['Circuit']
    if packet:
        options.legend.options.labels = ['Packet']
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
                                     ) for i in range(len(x))]
    options.output_fn = 'graphs/%s_vs_latency99.pdf' % fn
    options.x.label.xlabel = x_label
    options.y.label.ylabel = '99th percentile latency (us)'
    plot(x, y, options)


def graph_packet_util(data, x_label, fn, packet):
    x = [np.arange(len(zip(*data)[0]))]
    if not packet:
        y = [map(lambda j: j / (6.0/7 * 10) * 100, zip(*zip(*data)[1])[3])]
    else:
        y = [map(lambda j: j / 10 * 100, zip(*zip(*data)[1])[3])]
    print 'buffer:', x
    print 'packet:', y

    options = DotMap()
    options.plot_type = 'BAR'
    options.legend.options.labels = ['Strobe']
    options.legend.options.fontsize = 12
    options.bar_labels.format_string = '%1.2f'
    options.bar_labels.options.fontsize = 16
    options.output_fn = 'graphs/%s_vs_packet_util.pdf' % fn
    options.x.label.xlabel = x_label
    options.y.label.ylabel = 'Avg. packet utilization (%)'
    plot(x, y, options)


def graph_circuit_util(data, x_label, fn, circuit):
    x = [np.arange(len(zip(*data)[0]))]
    if not circuit:
        y = [map(lambda j: min(j / (0.9 * 1.0/7 * 80) * 100, 100.0),
                 zip(*zip(*data)[1])[4])]
    else:
        y = [map(lambda j: min(j / 80 * 100, 100.0), zip(*zip(*data)[1])[4])]
    print 'buffer:', x
    print 'circuit:', y

    options = DotMap()
    options.plot_type = 'BAR'
    # options.legend.options.labels = ['Strobe']
    options.legend.options.fontsize = 12
    options.bar_labels.format_string = '%1.0f'
    options.bar_labels.options.fontsize = 25
    if fn == 'buffer_size':
        options.x.ticks.major.labels = DotMap(
            text=[4, 8, 16, 32, 64, 128])
    if fn == 'days_out' or fn == 'ocs_days_out':
        options.x.ticks.major.labels = DotMap(
            text=[0, 200, 400, 600, 800, 1000, 1200, 1400])
    options.output_fn = 'graphs/%s_vs_circuit_util.pdf' % fn
    options.x.label.xlabel = x_label
    options.y.label.ylabel = 'Avg. circuit utilization (%)'
    options.y.ticks.major.show = False
    options.x.ticks.major.show = False
    plot(x, y, options)


def graph_rtts(data, x_label, fn):
    labels = zip(*data)[0]
    rtts = list(zip(*zip(*data)[1])[5])
    print rtts
    for i in xrange(len(rtts)):
        print rtts[i]
        rtts[i] = {k: v for (k, v) in rtts[i].items() if v > 1.0 and k < 3}
    x = [np.asarray(k.keys()) for k in rtts]
    y = [k.values() for k in rtts]
    print 'labels:', labels
    print 'rtts:', x
    print 'util:', y

    options = DotMap()
    options.plot_type = 'BAR'
    options.legend.options.labels = labels
    options.legend.options.fontsize = 12
    options.bar_labels.show = False
    options.output_fn = 'graphs/%s_rtts.pdf' % fn
    options.x.label.xlabel = 'RTTs since circuit start'
    options.y.label.ylabel = 'Avg. circuit utilization (%)'
    plot(x, y, options)

    
def graph_tput(data, x_label, fn):
    x = [zip(*data)[0]]
    y = [zip(*zip(*data)[1])[0]]
    print 'buffer:', x
    print 'tput:', y

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Strobe']
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1)
                              for i in range(len(x))]
    options.output_fn = 'graphs/%s_vs_tput.pdf' % fn
    options.x.label.xlabel = x_label
    options.y.label.ylabel = 'Rack throughput (Gbps)'
    plot(x, y, options)


if __name__ == '__main__':
    if not os.path.isdir(sys.argv[1]):
        print 'first arg must be dir'
        sys.exit(-1)
    db = shelve.open(sys.argv[1] + '/buffer_shelve.db')
    
    if 'circuit_data' in db:
        circuit_data = db['circuit_data']
    else:
        circuit_data = {}
        for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-circuit-*-'
                            'QUEUE-False-*-reno-*click.txt'):
            buffer_size = int(fn.split('/')[-1].split('circuit-')[1].split('-')[0])
            tput, lat, pack_util, circ_util, rtt_data, \
                cb, pb = get_tput_and_lat(fn)
            tput = tput[SR]
            lat50 = [x[1] for x in zip(*lat)[1]]
            lat99 = [x[1] for x in zip(*lat)[3]]
            circuit_data[buffer_size] = (tput, lat50, lat99, pack_util[SR],
                                         circ_util[SR],
                                         rtt_data[SR], float(cb) / pb)
        circuit_data = sorted(circuit_data.items())
        print circuit_data
        db['circuit_data'] = circuit_data

    graph(circuit_data, 'Buffer size (packets)', 'circuit_buffer_size',
          rtt=False, circuit=True)

    if 'packet_data' in db:
        packet_data = db['packet_data']
    else:
        packet_data = {}
        for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-no_circuit-*-'
                            'QUEUE-False-*-reno-*click.txt'):
            buffer_size = int(fn.split('/')[-1].split('no_circuit-')[1].split(
                '-')[0])
            tput, lat, pack_util, circ_util, rtt_data, \
                cb, pb = get_tput_and_lat(fn)
            tput = tput[SR]
            lat50 = [x[1] for x in zip(*lat)[1]]
            lat99 = [x[1] for x in zip(*lat)[3]]
            packet_data[buffer_size] = (tput, lat50, lat99, pack_util[SR],
                                        circ_util[SR],
                                        rtt_data[SR], float(cb) / pb)
        packet_data = sorted(packet_data.items())
        db['packet_data'] = packet_data
    print packet_data

    graph(packet_data, 'Buffer size (packets)', 'packet_buffer_size',
          rtt=False, packet=True)

    if 'buffer_data' in db:
        buffer_data = db['buffer_data']
    else:
        buffer_data = {}
        for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-*-'
                            'QUEUE-False-*-reno-*click.txt'):
            buffer_size = int(fn.split('/')[-1].split('strobe-')[1].split('-')[0])
            tput, lat, pack_util, circ_util, rtt_data, \
                cb, pb = get_tput_and_lat(fn)
            tput = tput[SR]
            lat50 = [x[1] for x in zip(*lat)[1]]
            lat99 = [x[1] for x in zip(*lat)[3]]
            buffer_data[buffer_size] = (tput, lat50, lat99, pack_util[SR],
                                        circ_util[SR],
                                        rtt_data[SR], float(cb) / pb)
        buffer_data = sorted(buffer_data.items())
        print buffer_data
        db['buffer_data'] = buffer_data
    
    graph(buffer_data, 'Buffer size (packets)', 'buffer_size')
    
    if 'days_out_data' in db:
        days_out_data = db['days_out_data']
    else:
        days_out_data = {0: buffer_data[2][1]}
        for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-16-'
                            'QUEUE-True-*-reno-*click.txt'):
            days_out = int(fn.split('/')[-1].split('True-')[1].split('-')[0]) / 20.0
            tput, lat, pack_util, circ_util, rtt_data, \
                cb, pb = get_tput_and_lat(fn)
            tput = tput[SR]
            lat50 = [x[1] for x in zip(*lat)[1]]
            lat99 = [x[1] for x in zip(*lat)[3]]
            days_out_data[days_out] = (tput, lat50, lat99, pack_util[SR],
                                       circ_util[SR], rtt_data[SR], float(cb) / pb)
        days_out_data = sorted(days_out_data.items())
        print days_out_data
        db['days_out_data'] = days_out_data

    graph(days_out_data, 'Early buffer resize ($\mu$s)', 'days_out')

    if 'ocs_strobe_data' in db:
        ocs_strobe_data = db['ocs_strobe_data']
    else:
        for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-16-'
                            'QUEUE-False-*-ocs-*click.txt'):
            tput, lat, pack_util, circ_util, rtt_data, \
                cb, pb = get_tput_and_lat(fn)
            tput = tput[SR]
            lat50 = [x[1] for x in zip(*lat)[1]]
            lat99 = [x[1] for x in zip(*lat)[3]]
            ocs_strobe_data = [(1, (tput, lat50, lat99, pack_util[SR],
                                    circ_util[SR], rtt_data[SR], float(cb) / pb))]
        print ocs_strobe_data
        db['ocs_strobe_data'] = ocs_strobe_data

    if 'ocs_days_out_data' in db:
        ocs_days_out_data = db['ocs_days_out_data']
    else:
        ocs_days_out_data = {0: ocs_strobe_data[0][1]}
        for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-16-'
                            'QUEUE-True-*-ocs-*click.txt'):
            days_out = int(fn.split('/')[-1].split('True-')[1].split('-')[0]) / 20.0
            tput, lat, pack_util, circ_util, rtt_data, \
                cb, pb = get_tput_and_lat(fn)
            tput = tput[SR]
            lat50 = [x[1] for x in zip(*lat)[1]]
            lat99 = [x[1] for x in zip(*lat)[3]]
            ocs_days_out_data[days_out] = (tput, lat50, lat99,
                                           pack_util[SR],
                                           circ_util[SR],
                                           rtt_data[SR], float(cb) / pb)
        ocs_days_out_data = sorted(ocs_days_out_data.items())
        print ocs_days_out_data
        db['ocs_days_out_data'] = ocs_days_out_data

    graph(ocs_days_out_data, 'Early buffer resize (us)', 'ocs_days_out')

    # # circuit v packet ratio, # of bytes
    # x = []
    # x.append(np.array(xrange(len(zip(*buffer_data)[0]))))
    # # x.append(np.array(xrange(len(zip(*days_out_data)[0]))))
    # # x.append(np.array(xrange(len(zip(*ocs_days_out_data)[0]))))
    # x = np.array(x)

    # y = []
    # y.append(zip(*zip(*buffer_data)[1])[6])
    # # y.append(zip(*zip(*days_out_data)[1])[6])
    # # y.append(zip(*zip(*ocs_days_out_data)[1])[6])
    # print 'bars:', x
    # print 'ratios:', y

    # options = DotMap()
    # options.plot_type = 'BAR'
    # options.legend.options.labels = ['Static buffer size', 'Resize days out',
    #                                  'TCP OCS', 'TCP OCS + Resize days out']
    # options.legend.options.fontsize = 12
    # options.output_fn = 'graphs/circuit_to_packet_ratio.pdf'
    # options.x.label.xlabel = ''
    # options.y.label.ylabel = 'Ratio of circuit to packet bytes'

    # plot(x, y, options)

    days_out_data = days_out_data[1:]
    ocs_days_out_data = ocs_days_out_data[1:]

    # throughput vs latency (50)
    x = []
    x.append(map(lambda j: min(j / (0.9 * 1.0/7 * 80) * 100, 100.0),
                 zip(*zip(*buffer_data)[1])[4]))
    x.append(map(lambda j: min(j / (0.9 * 1.0/7 * 80) * 100, 100.0),
                 zip(*zip(*days_out_data)[1])[4]))
    x.append(map(lambda j: min(j / (0.9 * 1.0/7 * 80) * 100, 100.0),
                 zip(*zip(*ocs_strobe_data)[1])[4]))
    x.append(map(lambda j: min(j / (0.9 * 1.0/7 * 80) * 100, 100.0),
                 zip(*zip(*ocs_days_out_data)[1])[4]))
    # x.append(zip(*zip(*buffer_data)[1])[0])
    # x.append(zip(*zip(*days_out_data)[1])[0])
    # x.append(zip(*zip(*ocs_strobe_data)[1])[0])
    # x.append(zip(*zip(*ocs_days_out_data)[1])[0])

    y = []
    y.append(zip(*zip(*zip(*buffer_data)[1])[1])[0])
    y.append(zip(*zip(*zip(*days_out_data)[1])[1])[0])
    y.append(zip(*zip(*zip(*ocs_strobe_data)[1])[1])[0])
    y.append(zip(*zip(*zip(*ocs_days_out_data)[1])[1])[0])
    print 'tput:', x
    print 'latency:', y

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Static buffers (vary size)', 'Dynamic buffers (vary $\\tau$)',
                                     'reTCP', 'reTCP + dynamic buffers (vary $\\tau$)']
    options.legend.options.fontsize = 19
    options.series_options = [DotMap(color='C%d' % i, marker='o', markersize=10, linewidth=5)
                              for i in range(len(x))]
    options.series_options[2].marker='x'
    options.series_options[2].s = 100
    del options.series_options[2].markersize
    options.series_options[2].zorder = 10
    options.output_fn = 'graphs/throughput_vs_latency.pdf'
    options.x.label.xlabel = 'Circuit utilization (%)'
    options.y.label.ylabel = 'Median latency ($\mu$s)'
    options.y.limits = [0, 600]
    options.y.ticks.major.labels = DotMap(
        locations=[0, 100, 200, 300, 400, 500, 600])

    plot(x, y, options)

    # throughput vs latency (99)
    x = []
    x.append(map(lambda j: min(j / (0.9 * 1.0/7 * 80) * 100, 100.0),
                 zip(*zip(*buffer_data)[1])[4]))
    x.append(map(lambda j: min(j / (0.9 * 1.0/7 * 80) * 100, 100.0),
                 zip(*zip(*days_out_data)[1])[4]))
    x.append(map(lambda j: min(j / (0.9 * 1.0/7 * 80) * 100, 100.0),
                 zip(*zip(*ocs_strobe_data)[1])[4]))
    x.append(map(lambda j: min(j / (0.9 * 1.0/7 * 80) * 100, 100.0),
                 zip(*zip(*ocs_days_out_data)[1])[4]))
    # x.append(zip(*zip(*buffer_data)[1])[0])
    # x.append(zip(*zip(*days_out_data)[1])[0])
    # x.append(zip(*zip(*ocs_strobe_data)[1])[0])
    # x.append(zip(*zip(*ocs_days_out_data)[1])[0])

    y = []
    y.append(zip(*zip(*zip(*buffer_data)[1])[2])[0])
    y.append(zip(*zip(*zip(*days_out_data)[1])[2])[0])
    y.append(zip(*zip(*zip(*ocs_strobe_data)[1])[2])[0])
    y.append(zip(*zip(*zip(*ocs_days_out_data)[1])[2])[0])
    print 'tput:', x
    print 'latency:', y

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Static buffers (vary size)', 'Dynamic buffers (vary $\\tau$)',
                                     'reTCP', 'reTCP + dynamic buffers (vary $\\tau$)']
    options.legend.options.fontsize = 19
    options.series_options = [DotMap(color='C%d' % i, marker='o', markersize=10, linewidth=5)
                              for i in range(len(x))]
    options.series_options[2].marker='x'
    # options.series_options[2].linewidth=5
    options.series_options[2].s = 100
    del options.series_options[2].markersize
    options.series_options[2].zorder = 10
    options.output_fn = 'graphs/throughput_vs_latency99.pdf'
    options.x.label.xlabel = 'Circuit utilization (%)'
    options.y.label.ylabel = '99th percent. latency ($\mu$s)'
    options.y.limits = [0, 1000]
    options.y.ticks.major.labels = DotMap(
        locations=[0, 200, 400, 600, 800, 1000])

    plot(x, y, options)

    db.close()
