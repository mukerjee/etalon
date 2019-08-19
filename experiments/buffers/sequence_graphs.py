#!/usr/bin/env python

from collections import defaultdict
import copy
import glob
from multiprocessing import Pool
import os
from os import path
import shelve
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For parse_logs.
sys.path.insert(0, path.join(PROGDIR, '..'))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, '..', '..', 'etc'))

from dotmap import DotMap
from simpleplotlib import plot

from parse_logs import get_seq_data
from python_config import TDF, CIRCUIT_BW_Gbps, PACKET_BW_Gbps

# Maps experiment to filename.
FILES = {
    'static': '*-strobe-*-False-*-reno-*click.txt',
    'resize': '*-QUEUE-True-*-reno-*click.txt',
}

# Maps experiment to a function that convert a filename to an integer key
# identifying this experiment (i.e., for the legend).
KEY_FN = {
    'static': lambda fn: int(fn.split('strobe-')[1].split('-')[0]),
    'resize': lambda fn: int(fn.split('True-')[1].split('-')[0]) / TDF,
}

UNITS = 1000.0  # Kilo-sequence number
NUM_HOSTS = 16.0


class FileReader(object):
    def __init__(self, name):
        self.name = name

    def __call__(self, fn):
        key = KEY_FN[self.name](fn.split('/')[-1])
        print fn, key
        return key, get_seq_data(fn)


def get_data(db, name):
    if name in db:
        return db[name]
    else:
        # data["raw_data"] = A list of pairs, where each pair corresponds to an
        #     experiment file.
        # data["raw_data"][i] = A pair of (key value, results).
        # data["raw_data"][i][1] = A pair of (list, n-tuple).
        # data["raw_data"][i][1][0] = A list of expected sequence number over
        #     time.
        # data["raw_data"][i][1][1] = An n-tuple of the times of circuit up/down
        #     events.
        # data["raw_data"][i][1][1][0] = The time at which the first day began.

        ptns = FILES[name]
        if not isinstance(ptns, list):
            ptns = [ptns]
        # For each pattern, extract the matches. Then, flatten them into a
        # single list.
        fns = [fn for matches in
                   [glob.glob(path.join(sys.argv[1], ptn)) for ptn in ptns]
               for fn in matches]

        assert len(fns) > 0, "Found no files for patterns: {}".format(ptns)
        print("Found files for patterns: {}".format(ptns))
        for fn in fns:
            print("    {}".format(fn))

        data = defaultdict(dict)
        p = Pool()
        data['raw_data'] = dict(p.map(FileReader(name), fns))
        # Clean up p.
        p.close()
        p.join()

        data['raw_data'] = sorted(data['raw_data'].items())
        data['keys'] = list(zip(*data['raw_data'])[0])
        data['lines'] = data['raw_data'][0][1][1]
        data['data'] = [map(lambda x: x / UNITS, f) for f in
                        zip(*zip(*data['raw_data'])[1])[0]]

        # Compute optimal sequence numbers. Recall that sequence numbers are in
        # terms of bytes.
        print("Computing optimal...")
        # Billions of total b/s -> total B/s -> total KB/s -> total KB/us
        #     -> per-host KB/us
        factor = 10**9 / 8. / UNITS / 10**6 / NUM_HOSTS
        pr_KBpus = PACKET_BW_Gbps * factor
        cr_KBpus = CIRCUIT_BW_Gbps * factor
        # Circuit start and end times, of the form:
        #     [<start>, <end>, <start>, <end>, ...]
        bounds = [int(round(q)) for q in data['lines']]
        assert len(bounds) % 2 == 0
        # Each entry is the optimal sequence number at that microsecond. The
        # optimal sequence number is the maximum amount of data that could have
        # been sent at a certain time.
        optimal = []
        print("bounds: {}".format(bounds))
        for state in xrange(0, len(bounds), 2):
            # Circuit night.
            if state == 0:
                # 0 to first day start.
                optimal = [pr_KBpus * us for us in xrange(bounds[state])]
            else:
                # Previous day end to current day start.
                optimal += [
                    optimal[-1] + pr_KBpus * us
                    for us in xrange(1, bounds[state] - bounds[state - 1] + 1)]
            # Current day start to current day end.
            optimal += [
                optimal[-1] + cr_KBpus * us
                for us in xrange(1, bounds[state + 1] - bounds[state] + 1)]

        # Bytes sent if we only used the packet network.
        pkt_only = [pr_KBpus * us for us in xrange(1, bounds[-1] + 1)]

        # Verify that in optimal, no two adjacent elements are equal.
        for i in xrange(bounds[-1] - 1):
            assert pkt_only[i] != pkt_only[i + 1], \
                "pkt_only[{}] == pkt_only[{}] == {}".format(
                    i, i + 1, pkt_only[i])
            assert optimal[i] != optimal[i + 1], \
                "optimal[{}] == optimal[{}] == {}".format(i, i + 1, optimal[i])

        data['keys'].insert(0, "packet only")
        data['data'].insert(0, pkt_only)
        data['keys'].insert(0, "optimal")
        data['data'].insert(0, optimal)
        return dict(data)


def plot_seq(data, fn, odr=path.join(PROGDIR, 'graphs'),
             ins=None, flt=lambda idx, label: True, order=None):
    x = [xrange(len(data['data'][i])) for i in xrange(len(data['keys']))]
    y = data['data']

    # Format the legend labels.
    lls = []
    for k in data['keys']:
        try:
            if 'static' in fn:
                lls += ['%s packets' % int(k)]
            else:
                lls += ['%s $\mu$s' % int(k)]
        except ValueError:
            lls += [k]

    options = DotMap()
    options.plot_type = 'LINE'
    options.legend.options.loc = "center right"
    options.legend.options.bbox_to_anchor = (1.25, 0.5)
    options.legend.options.labels = lls
    options.legend.options.fontsize = 14
    # Use 1 column if there are fewer than 4 lines, otherwise use 2 columns.
    options.legend.options.ncol = 1  # if len(data["data"]) < 4 else 2
    options.series_options = [DotMap(linewidth=2) for i in range(len(x))]
    options.output_fn = path.join(odr, 'seq_%s.pdf' % fn)
    options.x.label.xlabel = 'Time ($\mu$s)'
    options.y.label.ylabel = 'Expected seq. num.\n($\\times$1000)'
    options.x.label.fontsize = options.y.label.fontsize = 16
    options.x.ticks.major.options.labelsize = \
        options.y.ticks.major.options.labelsize = 16
    options.x.axis.show = options.y.axis.show = True
    options.x.axis.color = options.y.axis.color = 'black'
    lines = data['lines']
    options.vertical_lines.lines = lines
    shaded = []
    for i in xrange(0, len(lines), 2):
        shaded.append((lines[i], lines[i+1]))
    options.vertical_shaded.limits = shaded
    options.vertical_shaded.options.alpha = 0.1
    options.vertical_shaded.options.color = 'blue'

    if ins is not None:
        xlm, ylm = ins
        options.inset.show = True
        options.inset.options.zoom_level = 2
        options.inset.options.corners = [2, 3]
        options.inset.options.marker.options.color = 'black'
        options.inset.options.x.limits = xlm
        options.inset.options.y.limits = ylm


    # Pick only the lines that we want.
    x, y, options.legend.options.labels = zip(
        *[(a, b, l) for (i, (a, b, l)) in enumerate(
            zip(x, y, options.legend.options.labels))
          if flt(i, l)])

    if order is not None:
        real_x = []
        real_y = []
        real_l = []
        for item in order:
            idx = 0
            for possibility in options.legend.options.labels:
                if item in possibility:
                    break
                idx += 1
            real_x.append(x[idx])
            real_y.append(y[idx])
            real_l.append(options.legend.options.labels[idx])
        x = real_x
        y = real_y
        options.legend.options.labels = real_l

    plot(x, y, options)


if __name__ == '__main__':
    if not os.path.isdir(sys.argv[1]):
        print 'first arg must be dir'
        sys.exit(-1)
    db = shelve.open(path.join(sys.argv[1], 'seq_shelve.db'))
    db['static'] = get_data(db, 'static')
    plot_seq(db['static'], 'static')

    db['resize'] = get_data(db, 'resize')
    resize_data = copy.copy(db['resize'])
    # Use the same circuit windows as the 'static' experiment.
    resize_data['lines'] = db['static']['lines']
    # Use the data for 0 us from the 'static' experiment.
    resize_data['keys'] = [0] + db['resize']['keys']
    resize_data['data'] = [db['static']['data'][2]] + db['resize']['data']
    plot_seq(resize_data, 'resize')

    db.close()
