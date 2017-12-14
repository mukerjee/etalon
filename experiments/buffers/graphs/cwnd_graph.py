#!/usr/bin/env python

import sys

import matplotlib.pyplot as plt

datas = [[], []]


def get_data(fn):
    current = {}
    start_ts = -1
    data = [[], []]
    for line in open(fn):
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

if __name__ == '__main__':
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
