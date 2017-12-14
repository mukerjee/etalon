#!/usr/bin/env python

import sys
import matplotlib.pyplot as plt

datas = [[], []]


def get_data(fn):
    start_ts = -1
    data = [[], []]
    for l in open(fn):
        l = l.split(':')
        if len(l) != 2:
            break
        ts, d = l
        d = d.split(',')
        if len(d) != 5:
            break
        occ, length = map(lambda x: int(x), d[2].split())
        ts = float(ts)
        if start_ts == -1:
            start_ts = ts
        data[0].append((ts - start_ts, occ))
        data[1].append((ts - start_ts, length))
    return data

for fn in sys.argv[1:]:
    d = get_data(fn)
    datas[0].append(zip(*d[0])[0])
    datas[1].append(zip(*d[0])[1])

x = datas[0]
y = datas[1]

if __name__ == '__main__':
    if len(x) == 1:
        plt.plot(x[0], y[0], 'r-')
    if len(x) == 2:
        plt.plot(x[0], y[0], 'r-', x[1], y[1], 'b-')
    if len(x) == 3:
        plt.plot(x[0], y[0], 'r-', x[1], y[1], 'b-', x[2], y[2], 'g-')
    if len(x) == 4:
        plt.plot(x[0], y[0], 'r-', x[1], y[1], 'b-', x[2], y[2], 'g-',
                 x[3], y[3], 'y-')
    plt.show()
