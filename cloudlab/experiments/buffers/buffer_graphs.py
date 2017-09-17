#!/usr/bin/env python

from dotmap import DotMap

import sys
sys.path.append('../')
sys.path.insert(0, '/Users/mukerjee/Dropbox/Research/simpleplotlib/')

from graph_common import get_tput_and_lat
from simpleplotlib import plot


experiment_names = [
    # ('one_to_one', 1505606782),
    # ('pair', 1505607352),
    # ('one_to_two', 1505607933),
    # ('two_to_one', 1505608526),
    # ('all_pairs', 1505610857),
    # ('ring', 1505611543),
    # ('one_to_all', 1505612558),
    ('one_to_all', 1505618353),
    # ('all_to_one', 1505618997),
    # ('all_to_one', 1505553758),
    # ('all_to_all', 1505605145)
]

# pattern_labels = ['One to One', 'Pair', 'One to Two',
#                   'Two to One', 'All Pairs', 'Ring',
#                   'One to All', 'All to One']
pattern_labels = ['One to All', 'All to One']
schedule_labels = ['Solstice', 'No Circuit', 'Strobe', 'Resize']
config_types = ['normal', 'resize']  # 'no_circuit', 'strobe']
# buffer_sizes = [1, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50]
buffer_sizes = [1, 5, 10, 20, 30, 40, 50, 75, 100]#, 250, 500, 750, 1000]
# buffer_sizes = [75]

default_config = {'type': 'normal', 'buffer_size': 40,
                  'traffic_source': 'QUEUE', 'queue_resize': False}

for ct in config_types:
    config = dict(default_config)
    config['type'] = ct
    if ct == 'resize':
        config['type'] = 'normal'
        config['queue_resize'] = True

    # print config

    tputs = []
    tput_std = []
    pings = []
    ping_std = []
    tputs_down = []
    tput_down_std = []
    pings90 = []
    pings99 = []

    for pattern, t in experiment_names:
        tputs.append([])
        tput_std.append([])
        pings.append([])
        ping_std.append([])
        tputs_down.append([])
        tput_down_std.append([])
        pings90.append([])
        pings99.append([])
        for b in buffer_sizes:
            config['buffer_size'] = b
            (tput, tpstd), (ping, pstd), \
                (tput_down, tpdstd), \
                (ping90, ping99) = get_tput_and_lat(pattern, t, config)
            tputs[-1].append(tput)
            tput_std[-1].append(tpstd)
            pings[-1].append(ping)
            ping_std[-1].append(pstd)
            tputs_down[-1].append(tput_down)
            tput_down_std[-1].append(tpdstd)
            pings90[-1].append(ping90)
            pings99[-1].append(ping99)

    print config
    print 'buffer: ', buffer_sizes
    print 'tputs_up: ', tputs
    print 'tputs_down: ', tputs_down
    print 'latency: ', pings
    print 'latency (90): ', pings90
    print 'latency (99): ', pings99
    continue

    # size vs latency
    x = [buffer_sizes for b in range(len(experiment_names))]
    y = pings
    yerr = ping_std
    print 'buffer:', x
    print 'latency:', y
    print 'lat std:', yerr

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = pattern_labels
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
                                     yerr=yerr[i]) for i in range(len(x))]
    options.output_fn = 'graphs/%s-buffer_size_vs_latency.pdf' % ct
    options.x.label.xlabel = 'Buffer size (packets)'
    options.y.label.ylabel = 'Latency (us)'
    plot(x, y, options)

    # size vs latency (90)
    x = [buffer_sizes for b in range(len(experiment_names))]
    y = pings90
    print 'buffer:', x
    print '90% latency:', y

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = pattern_labels
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
                                     ) for i in range(len(x))]
    options.output_fn = 'graphs/%s-buffer_size_vs_latency90.pdf' % ct
    options.x.label.xlabel = 'Buffer size (packets)'
    options.y.label.ylabel = 'Latency (us)'
    plot(x, y, options)

    # size vs latency (99)
    x = [buffer_sizes for b in range(len(experiment_names))]
    y = pings99
    print 'buffer:', x
    print 'latency:', y

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = pattern_labels
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
                                     ) for i in range(len(x))]
    options.output_fn = 'graphs/%s-buffer_size_vs_latency99.pdf' % ct
    options.x.label.xlabel = 'Buffer size (packets)'
    options.y.label.ylabel = 'Latency (us)'
    plot(x, y, options)


    # size vs throughput (up)
    x = [buffer_sizes for b in range(len(experiment_names))]
    y = tputs
    yerr = tput_std
    print 'buffer:', x
    print 'tput:', y
    print 'tput std:', yerr

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = pattern_labels
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
                                     yerr=yerr[i]) for i in range(len(x))]
    options.output_fn = 'graphs/%s-bufer_size_vs_up_util.pdf' % ct
    options.x.label.xlabel = 'Buffer size (packets)'
    # options.y.label.ylabel = 'Rack throughput (Gbps)'
    options.y.label.ylabel = 'Avg. Cir. Up Link Util. (%)'
    # options.y.limits = [0, 10]

    plot(x, y, options)

    
    # size vs throughput (down)
    x = [buffer_sizes for b in range(len(experiment_names))]
    y = tputs_down
    yerr = tput_down_std
    print 'buffer:', x
    print 'tput_down:', y
    print 'tput_down std:', yerr

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.labels = pattern_labels
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
                                     yerr=yerr[i]) for i in range(len(x))]
    options.output_fn = 'graphs/%s-bufer_size_vs_down_util.pdf' % ct
    options.x.label.xlabel = 'Buffer size (packets)'
    # options.y.label.ylabel = 'Rack throughput (Gbps)'
    options.y.label.ylabel = 'Avg. Cir. Down Link Util. (%)'
    # options.y.limits = [0, 10]

    plot(x, y, options)

    # throughput vs latency
    x = tputs
    y = pings
    xerr = tput_std
    yerr = ping_std
    print 'tput:', x
    print 'latency:', y
    print 'tput std:', xerr
    print 'lat std:', yerr

    options = DotMap()
    options.plot_type = 'SCATTER'
    options.legend.options.labels = pattern_labels
    options.legend.options.fontsize = 12
    options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1)
                              for i in range(len(x))]
    options.output_fn = 'graphs/%s-throughput_vs_latency.pdf' % ct
    # options.x.label.xlabel = 'Rack throughput (Gbps)'
    options.x.label.xlabel = 'Avg. Cir. Up Link Util. (%)'
    options.y.label.ylabel = 'Latency (us)'

    plot(x, y, options)











# for pattern, t in experiment_names:

#     tputs = []
#     tput_std = []
#     pings = []
#     ping_std = []

#     for ct in config_types:
#         config = dict(default_config)
#         config['type'] = ct
#         if ct == 'resize':
#             config['type'] = 'normal'
#             config['queue_resize'] = True

#         tputs.append([])
#         tput_std.append([])
#         pings.append([])
#         ping_std.append([])
#         for b in buffer_sizes:
#             config['buffer_size'] = b
#             (tput, tpstd), (ping, pstd) = get_tput_and_lat(pattern, t, config)
#             tputs[-1].append(tput)
#             tput_std[-1].append(tpstd)
#             pings[-1].append(ping)
#             ping_std[-1].append(pstd)

#     # size vs latency
#     x = [buffer_sizes for b in range(len(config_types))]
#     y = pings
#     yerr = ping_std
#     print 'buffer:', x
#     print 'latency:', y
#     print 'lat std:', yerr

#     options = DotMap()
#     options.plot_type = 'LINE'
#     options.legend.options.labels = schedule_labels
#     options.legend.options.fontsize = 12
#     options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
#                                      yerr=yerr[i]) for i in range(len(x))]
#     options.output_fn = 'graphs/%s-buffer_size_vs_latency.pdf' % pattern
#     options.x.label.xlabel = 'Buffer size (packets)'
#     options.y.label.ylabel = 'Latency (us)'
#     plot(x, y, options)


#     # size vs throughput
#     x = [buffer_sizes for b in range(len(config_types))]
#     y = tputs
#     yerr = tput_std
#     print 'buffer:', x
#     print 'tput:', y
#     print 'tput std:', yerr

#     options = DotMap()
#     options.plot_type = 'LINE'
#     options.legend.options.labels = schedule_labels
#     options.legend.options.fontsize = 12
#     options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1,
#                                      yerr=yerr[i]) for i in range(len(x))]
#     options.output_fn = 'graphs/%s-bufer_size_vs_throughput.pdf' % pattern
#     options.x.label.xlabel = 'Buffer size (packets)'
#     options.y.label.ylabel = 'Rack throughput (Gbps)'
#     # options.y.limits = [0, 10]

#     plot(x, y, options)


#     # throughput vs latency
#     x = tputs
#     y = pings
#     xerr = tput_std
#     yerr = ping_std
#     print 'tput:', x
#     print 'latency:', y
#     print 'tput std:', xerr
#     print 'lat std:', yerr

#     options = DotMap()
#     options.plot_type = 'SCATTER'
#     options.legend.options.labels = schedule_labels
#     options.legend.options.fontsize = 12
#     options.series_options = [DotMap(color='C%d' % i, marker='o', linewidth=1)
#                               for i in range(len(x))]
#     options.output_fn = 'graphs/%s-throughput_vs_latency.pdf' % pattern
#     options.x.label.xlabel = 'Rack throughput (Gbps)'
#     options.y.label.ylabel = 'Latency (us)'

#     plot(x, y, options)
