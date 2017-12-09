#!/usr/bin/env python

import sys

# from dotmap import DotMap
# from simpleplotlib import plot
import matplotlib.pyplot as plt

datas = [[], []]


def get_data(fn):
    current = {}
    start_ts = -1
    data = [[], []]
    for line in open(fn):
        # if '10.1.1' not in line and '10.1.10.1' not in line:
        #     continue
        ts, src, dst, bytes, send_seq, unack_seq, cwnd, \
            ssthresh, send_wnd, rtt, rwnd = line.split()
        if '10.1.10.1' not in src and '10.1.1' not in src:
            continue
        ts = float(ts)
        if start_ts == -1:
            start_ts = ts
        cwnd = int(cwnd)
        current[src] = cwnd
        ssthresh = int(ssthresh)
        ssthresh = ssthresh if ssthresh < 2147483647 else 0
        # data[0].append((ts - start_ts, cwnd))
        data[0].append((ts - start_ts, sum(current.values())))
        data[1].append((ts - start_ts, ssthresh))
    return data

for fn in sys.argv[1:]:
    d = get_data(fn)
    print len(d[0])
    datas[0].append(zip(*d[0])[0])
    datas[1].append(zip(*d[0])[1])

x = datas[0]
y = datas[1]

# x = [zip(*d)[0] for d in datas]
# y = [zip(*d)[1] for d in datas]

# options = DotMap()

# options.output_fn = 'cwnd_graph.pdf'

# options.plot_type = 'LINE'

# options.legend.options.labels = ['snd_cwnd', 'snd_ssthresh']
# options.legend.options.fontsize = 14

# options.x.label.xlabel = 'time (seconds)'
# options.y.label.ylabel = 'segments'

if __name__ == '__main__':
    # plt.plot(zip(x[0], x[1]), zip(y[0], y[1]))
    if len(x) == 1:
        plt.plot(x[0], y[0], 'p-')
    if len(x) == 2:
        plt.plot(x[0], y[0], 'pr-', x[1], y[1], 'pb-')
    if len(x) == 3:
        plt.plot(x[0], y[0], 'pr-', x[1], y[1], 'pb-', x[2], y[2], 'pg-')
    if len(x) == 4:
        plt.plot(x[0], y[0], 'pr-', x[1], y[1], 'pb-', x[2], y[2], 'pg-',
                 x[3], y[3], 'py-')
    if len(x) == 5:
        plt.plot(x[0], y[0], 'r-', x[1], y[1], 'b-', x[2], y[2], 'g-',
                 x[3], y[3], 'y-', x[4], y[4], 'p-')
    plt.show()
    # plot(x, y, options)
