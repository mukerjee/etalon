#!/usr/bin/env python

from dotmap import DotMap

import sys
sys.path.insert(0, '/Users/mukerjee/Dropbox/Research/simpleplotlib/')

import glob
import numpy as np

from get_throughput_and_latency import get_tput_and_lat
from simpleplotlib import plot


def graph(data, x_label, fn):
    graph_lat50(data, x_label, fn)
    graph_lat99(data, x_label, fn)
    graph_tput(data, x_label, fn)
    graph_packet_util(data, x_label, fn)
    graph_circuit_util(data, x_label, fn)
    graph_rtts(data, x_label, fn)


def graph_lat50(data, x_label, fn):
    # size vs latency
    x = [zip(*data)[0]]
    y = [zip(*zip(*data)[1])[1]]
    # yerr = ping_std
    print 'buffer:', x
    print 'latency:', y
    # print 'lat std:', yerr

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Strobe']
    options.legend.options.fontsize = 12
    # options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
    #                                  yerr=yerr[i]) for i in range(len(x))]
    options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1)
                              for i in range(len(x))]
    options.output_fn = 'graphs/%s_vs_latency.pdf' % fn
    options.x.label.xlabel = x_label
    options.y.label.ylabel = 'Median latency (us)'
    plot(x, y, options)


def graph_lat99(data, x_label, fn):
    # size vs latency (99)
    x = [zip(*data)[0]]
    y = [zip(*zip(*data)[1])[2]]
    print 'buffer:', x
    print 'latency:', y

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Strobe']
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
                                     ) for i in range(len(x))]
    options.output_fn = 'graphs/%s_vs_latency99.pdf' % fn
    options.x.label.xlabel = x_label
    options.y.label.ylabel = '99th percentile latency (us)'
    plot(x, y, options)


def graph_packet_util(data, x_label, fn):
    x = [np.arange(len(zip(*data)[0]))]
    y = [map(lambda j: j / (6.0/7 * 10) * 100, zip(*zip(*data)[1])[3])]
    print 'buffer:', x
    print 'packet:', y

    options = DotMap()
    options.plot_type = 'BAR'
    options.legend.options.labels = ['Strobe']
    options.legend.options.fontsize = 12
    options.bar_labels.format_string = '%1.2f'
    options.bar_labels.options.fontsize = 16
    # options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
    #                                  ) for i in range(len(x))]
    options.output_fn = 'graphs/%s_vs_packet_util.pdf' % fn
    options.x.label.xlabel = x_label
    options.y.label.ylabel = 'Avg. packet utilization (%)'
    plot(x, y, options)


def graph_circuit_util(data, x_label, fn):
    x = [np.arange(len(zip(*data)[0]))]
    y = [map(lambda j: j / (0.9 * 1.0/7 * 80) * 100, zip(*zip(*data)[1])[4])]
    print 'buffer:', x
    print 'circuit:', y

    options = DotMap()
    options.plot_type = 'BAR'
    options.legend.options.labels = ['Strobe']
    options.legend.options.fontsize = 12
    options.bar_labels.format_string = '%1.2f'
    options.bar_labels.options.fontsize = 16
    # options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
    #                                  ) for i in range(len(x))]
    options.output_fn = 'graphs/%s_vs_circuit_util.pdf' % fn
    options.x.label.xlabel = x_label
    options.y.label.ylabel = 'Avg. circuit utilization (%)'
    plot(x, y, options)


def graph_rtts(data, x_label, fn):
    labels = zip(*data)[0]
    rtts = list(zip(*zip(*data)[1])[5])
    print rtts
    for i in xrange(len(rtts)):
        print rtts[i]
        rtts[i] = {k: v for (k, v) in rtts[i].items() if k < 6}
    # rtts = [{k: v for (k, v) in r if k < 6} for r in rtts]
    x = [np.asarray(k.keys()) for k in rtts]
    y = [k.values() for k in rtts]
    print 'labels:', labels
    print 'rtts:', x
    print 'util:', y

    options = DotMap()
    options.plot_type = 'BAR'
    options.legend.options.labels = labels
    options.legend.options.fontsize = 12
    # options.bar_labels.format_string = '%1.2f'
    # options.bar_labels.options.fontsize = 16
    options.bar_labels.show = False
    # options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
    #                                  ) for i in range(len(x))]
    options.output_fn = 'graphs/%s_rtts.pdf' % fn
    options.x.label.xlabel = 'RTTs since circuit start'
    options.y.label.ylabel = 'Avg. circuit utilization (%)'
    plot(x, y, options)

    
def graph_tput(data, x_label, fn):
    # size vs throughput
    x = [zip(*data)[0]]
    y = [zip(*zip(*data)[1])[0]]
    # yerr = tput_std
    print 'buffer:', x
    print 'tput:', y
    # print 'tput std:', yerr

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Strobe']
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1)
                              for i in range(len(x))]
    # options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
    #                                  yerr=yerr[i]) for i in range(len(x))]
    options.output_fn = 'graphs/%s_vs_tput.pdf' % fn
    options.x.label.xlabel = x_label
    options.y.label.ylabel = 'Rack throughput (Gbps)'
    plot(x, y, options)


if __name__ == '__main__':
    buffer_data = {}
    for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-*-'
                        'QUEUE-False-*-reno-click.txt'):
    # for fn in glob.glob('buffer-data-12-14/tmp/strobe-buffer-size/*.txt'):
        buffer_size = int(fn.split('/')[-1].split('strobe-')[1].split('-')[0])
        tput, lat, pack_util, circ_util, rtt_data = get_tput_and_lat(fn)
        tput = tput.values()[0]
        lat50 = lat[1][1]
        lat99 = lat[3][1]
        buffer_data[buffer_size] = (tput, lat50, lat99, pack_util.values()[0],
                                    circ_util.values()[0], rtt_data.values()[0])
    buffer_data = sorted(buffer_data.items())
    print buffer_data
    
    graph(buffer_data, 'Buffer size (packets)', 'buffer_size')

    days_out_data = {}
    for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-16-'
                        'QUEUE-True-*-reno-click.txt'):
    # for fn in glob.glob('buffer-data-12-14/tmp/resize-days-out/*.txt'):
        days_out = int(fn.split('/')[-1].split('True-')[1].split('-')[0])
        tput, lat, pack_util, circ_util, rtt_data = get_tput_and_lat(fn)
        tput = tput.values()[0]
        lat50 = lat[1][1]
        lat99 = lat[3][1]
        days_out_data[days_out] = (tput, lat50, lat99, pack_util.values()[0],
                                   circ_util.values()[0], rtt_data.values()[0])
    days_out_data = sorted(days_out_data.items())
    print days_out_data
    graph(days_out_data, 'Early buffer resize (us)', 'days_out')

    for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-16-'
                        'QUEUE-False-*-ocs-click.txt'):
    # for fn in glob.glob('buffer-data-12-14/tmp/ocs-strobe/*.txt'):
        tput, lat, pack_util, circ_util, rtt_data = get_tput_and_lat(fn)
        tput = tput.values()[0]
        lat50 = lat[1][1]
        lat99 = lat[3][1]
        ocs_strobe_data = [(1, (tput, lat50, lat99, pack_util.values()[0],
                                circ_util.values()[0], rtt_data.values()[0]))]
    print ocs_strobe_data
    graph(ocs_strobe_data, 'None', 'ocs_strobe')

    ocs_days_out_data = {}
    for fn in glob.glob(sys.argv[1] + '/tmp/*-one_to_one-strobe-16-'
                        'QUEUE-True-*-ocs-click.txt'):
    # for fn in glob.glob('buffer-data-12-14/tmp/ocs-days-out/*.txt'):
        days_out = int(fn.split('/')[-1].split('True-')[1].split('-')[0])
        tput, lat, pack_util, circ_util, rtt_data = get_tput_and_lat(fn)
        tput = tput.values()[0]
        lat50 = lat[1][1]
        lat99 = lat[3][1]
        ocs_days_out_data[days_out] = (tput, lat50, lat99,
                                       pack_util.values()[0],
                                       circ_util.values()[0],
                                       rtt_data.values()[0])
    ocs_days_out_data = sorted(ocs_days_out_data.items())
    print ocs_days_out_data
    graph(ocs_days_out_data, 'Early buffer resize (us)', 'ocs_days_out')



    # throughput vs latency (50)
    x = []
    x.append(zip(*zip(*buffer_data)[1])[0])
    x.append(zip(*zip(*days_out_data)[1])[0])
    x.append(zip(*zip(*ocs_strobe_data)[1])[0])
    x.append(zip(*zip(*ocs_days_out_data)[1])[0])

    y = []
    y.append(zip(*zip(*buffer_data)[1])[1])
    y.append(zip(*zip(*days_out_data)[1])[1])
    y.append(zip(*zip(*ocs_strobe_data)[1])[1])
    y.append(zip(*zip(*ocs_days_out_data)[1])[1])
    # xerr = tput_std
    # yerr = ping_std
    print 'tput:', x
    print 'latency:', y
    # print 'tput std:', xerr
    # print 'lat std:', yerr

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Static buffer size', 'Resize days out',
                                     'TCP OCS', 'TCP OCS + Resize days out']
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1)
                              for i in range(len(x))]
    options.output_fn = 'graphs/throughput_vs_latency.pdf'
    options.x.label.xlabel = 'Rack throughput (Gbps)'
    # options.x.label.xlabel = 'Avg. Cir. Up Link Util. (%)'
    options.y.label.ylabel = 'Median latency (us)'

    plot(x, y, options)


    # throughput vs latency (99)
    x = []
    x.append(zip(*zip(*buffer_data)[1])[0])
    x.append(zip(*zip(*days_out_data)[1])[0])
    x.append(zip(*zip(*ocs_strobe_data)[1])[0])
    x.append(zip(*zip(*ocs_days_out_data)[1])[0])

    y = []
    y.append(zip(*zip(*buffer_data)[1])[2])
    y.append(zip(*zip(*days_out_data)[1])[2])
    y.append(zip(*zip(*ocs_strobe_data)[1])[2])
    y.append(zip(*zip(*ocs_days_out_data)[1])[2])
    # xerr = tput_std
    # yerr = ping_std
    print 'tput:', x
    print 'latency:', y
    # print 'tput std:', xerr
    # print 'lat std:', yerr

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = ['Static buffer size', 'Resize days out',
                                     'TCP OCS', 'TCP OCS + Resize days out']
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1)
                              for i in range(len(x))]
    options.output_fn = 'graphs/throughput_vs_latency99.pdf'
    options.x.label.xlabel = 'Rack throughput (Gbps)'
    # options.x.label.xlabel = 'Avg. Cir. Up Link Util. (%)'
    options.y.label.ylabel = '99th percentile latency (us)'

    plot(x, y, options)
