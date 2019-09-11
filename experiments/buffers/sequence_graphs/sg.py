#!/usr/bin/env python

from collections import defaultdict
import copy
import glob
from multiprocessing import Pool
from os import path
import shelve
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For parse_logs.
sys.path.insert(0, path.join(PROGDIR, '..', '..'))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, '..', '..', '..', 'etc'))

from dotmap import DotMap
from simpleplotlib import plot

import parse_logs
from python_config import TDF, CIRCUIT_BW_Gbps, PACKET_BW_Gbps

# Maps experiment to filename.
FILES = {
    'static': '*-fixed-*-False-*-reno-*click.txt',
    'resize': '*-QUEUE-True-*-reno-*click.txt',
}
# Maps experiment to a function that convert a filename to an integer key
# identifying this experiment (i.e., for the legend).
KEY_FN = {
    'static': lambda fn: int(fn.split('fixed-')[1].split('-')[0]),
    'resize': lambda fn: int(fn.split('True-')[1].split('-')[0]) / TDF,
}
# Kilo-sequence number
UNITS = 1000.0
NUM_HOSTS = 16.0


class FileReader(object):
    def __init__(self, name):
        self.name = name

    def __call__(self, fn):
        key = KEY_FN[self.name](fn.split('/')[-1])
        print fn, key
        return key, parse_logs.get_seq_data(fn)


def add_optimal(data):
    """
    Adds the calculated baselines (optimal and packet-only) to the provided data
    dictionary.
    """
    # Compute optimal sequence numbers. Recall that sequence numbers are in
    # terms of bytes, so we use bytes in some of the below comments.
    print("Computing optimal...")

    # Calculate the raw rate of the packet and circuit networks.
    #
    # Constant used to convert Gb/s to the units of the graphs. E.g., using
    # NUM_HOSTS = 16 and UNITS = KB, our target is KB / us / host:
    #
    #                                 div by    div by
    #                                NUM_HOSTS  UNITS
    #
    #     10**9 b    1 B      1 s     1 rack     1 KB    0.0078125 KB
    #     ------- x  --- x -------- x ------- x ------ = ------------
    #       Gb       8 b   10**6 us   16 host   1000 B    us x host
    factor = 10**9 / 8. / 10**6 / NUM_HOSTS / UNITS
    pr_KBpus = PACKET_BW_Gbps * factor
    cr_KBpus = CIRCUIT_BW_Gbps * factor

    # Circuit start and end times, of the form:
    #     [<start>, <end>, <start>, <end>, ...]
    bounds = [int(round(q)) for q in data['lines']]
    assert len(bounds) % 2 == 0, \
        ("Circuit starts and ends must come in pairs, but the list of them "
         "contains an odd number of elements: {}".format(bounds))
    print("circuit bounds: {}".format(bounds))

    # Each entry is the maximum amount of data that could have been sent by that
    # time.
    optimal = []
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
    pkt_only = [pr_KBpus * us for us in xrange(0, bounds[-1])]

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


def get_data(db, key):
    """
    (Optionally) loads the results for the specified key into the provided
    database and returns a copy of them.
    """
    if key not in db:
        # data["raw_data"] = A list of pairs, where each pair corresponds to an
        #     experiment file.
        # data["raw_data"][i] = A pair of (key value, results).
        # data["raw_data"][i][1] = A pair of (list, n-tuple).
        # data["raw_data"][i][1][0] = A list of expected sequence number over
        #     time.
        # data["raw_data"][i][1][1] = An n-tuple of the times of circuit up/down
        #     events.
        # data["raw_data"][i][1][1][0] = The time at which the first day began.

        ptns = FILES[key]
        if not isinstance(ptns, list):
            ptns = [ptns]
        # For each pattern, extract the matches. Then, flatten them into a
        # single list.
        fns = [fn for matches in
               [glob.glob(path.join(sys.argv[1], ptn)) for ptn in ptns]
               for fn in matches]

        assert fns, "Found no files for patterns: {}".format(ptns)
        print("Found files for patterns: {}".format(ptns))
        for fn in fns:
            print("    {}".format(fn))

        data = defaultdict(dict)
        p = Pool()
        data['raw_data'] = dict(p.map(FileReader(key), fns))
        # Clean up p.
        p.close()
        p.join()

        data['raw_data'] = sorted(data['raw_data'].items())
        data['keys'] = list(zip(*data['raw_data'])[0])
        data['lines'] = data['raw_data'][0][1][1]
        data['data'] = [map(lambda x: x / UNITS, f) for f in
                        zip(*zip(*data['raw_data'])[1])[0]]

        # Store the new data in the database.
        db[key] = data

    data = copy.deepcopy(db[key])
    add_optimal(data)
    return data


def plot_seq(data, fn, odr=path.join(PROGDIR, '..', 'graphs'),
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
    options.legend.options.bbox_to_anchor = (1.4, 0.5)
    options.legend.options.labels = lls
    options.legend.options.fontsize = 18
    # Use 1 column if there are fewer than 4 lines, otherwise use 2 columns.
    options.legend.options.ncol = 1  # if len(data["data"]) < 4 else 2
    options.series_options = [DotMap(linewidth=2) for i in range(len(x))]
    options.output_fn = path.join(odr, 'seq_%s.pdf' % fn)
    options.x.label.xlabel = 'Time ($\mu$s)'
    options.y.label.ylabel = 'Expected seq. num.\n($\\times$1000)'
    options.x.label.fontsize = options.y.label.fontsize = 18
    options.x.ticks.major.options.labelsize = \
        options.y.ticks.major.options.labelsize = 18
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
        options.inset.options.zoom_level = 2.5
        options.inset.options.corners = [2, 3]
        options.inset.options.marker.options.color = 'black'
        options.inset.options.x.limits = xlm
        options.inset.options.y.limits = ylm

    # Pick only the lines that we want.
    if flt is not None:
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
            found = False
            for possibility in options.legend.options.labels:
                if item in possibility:
                    found = True
                    break
                idx += 1
            if found:
                real_x.append(x[idx])
                real_y.append(y[idx])
                real_l.append(options.legend.options.labels[idx])
        x = real_x
        y = real_y
        options.legend.options.labels = real_l

    plot(x, y, options)


def rst_glb(dur):
    """ Reset global variables. """
    # Reset global lookup tables.
    FILES = {}
    KEY_FN = {}
    # Reset experiment duration.
    parse_logs.DURATION = dur
    # Do not set sg.DURATION because it get configured automatically based on
    # the actual circuit timings.


def seq(name, edr, odr, ptn, key_fnc, dur, ins=None, flt=None, order=None):
    """ Create a sequence graph.

    name: Name of this experiment, which become the output filename.
    edr: Experiment dir.
    odr: Output dir.
    ptn: Glob pattern for experiment files.
    key_fnc: Function that takes an experiment data filename returns a legend
             key.
    dur: The duration of the experiment, in milliseconds.
    ins: An inset specification.
    flt: Function that takes a legend index and label and returns a boolean
         indicating whether to include that line.
    order: List of the legend labels in their desired order.
    """
    print("Plotting: {}".format(name))
    rst_glb(dur)
    # Names are of the form "<number>_<details>_<specific options>". Experiments
    # where <details> are the same should be based on the same data. Therefore,
    # use <details> as the database key.
    basename = name.split("_")[1]
    FILES[basename] = ptn
    KEY_FN[basename] = key_fnc
    db = shelve.open(path.join(edr, "{}.db".format(basename)))
    plot_seq(get_data(db, basename), name, odr, ins, flt, order)
    db.close()
