#!/usr/bin/env python
#
# This program generates graphs for experiments produced by
# etalon/experiments/buffers/nsdi2020.py.

import os
from os import path
import shelve
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For parse_logs.
sys.path.insert(0, path.join(PROGDIR, '..'))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "..", "etc"))
# For sg.
sys.path.insert(0, path.join(PROGDIR, "sequence_graphs"))

import parse_logs
import python_config
import sg

# Filename patterns.
#
# Matches experiments with a particular CC mode, 1000 us nights, and 9000 us
# days (under TDF).
OLD_PTN = "*-{}-*-20000-180000-click.txt"
# Matches experiments with a particular CC mode, 0.1 us nights, and 0.9 us days
# (under TDF).
FUTURE_PTN = "*-{}-*-2-18-click.txt"
# Matches experiments with static buffers of a particular size, a particular CC
# mode, 20 us nights, and 180 us days (under TDF).
STATIC_PTN = "*-{}-QUEUE-False-*-{}-*-400-3600-click.txt"
# Matches experiments with dynamic buffers, a particular resize time, and a
# particular CC mode.
DYN_PTN = "*-QUEUE-True-{}-{}-*click.txt"

# Inset window bounds.
DYN_INS = ((620, 800), (50, 275))
# CC mode indices to display in static graph.
DESIRED_CCS = ["optimal", "packet only", "bbr", "cubic", "dctcp", "highspeed",
               "illinois", "scalable", "westwood", "yeah"]
# Resize time indices to display in resize graph.
DESIRED_RESIZE_US = [0, 1, 3, 5, 7, 8, 9, 11, 12]
# Resize time to graph for reTCP.
CHOSEN_RESIZE_US = int(175 * python_config.TDF)
# The TCP variant to use as our baseline.
CHOSEN_TCP = "cubic"
# Static buffer size to use.
CHOSEN_STATIC = 16


def rst_glb(dur):
    """ Reset global variables. """
    # Reset global lookup tables.
    sg.FILES = {}
    sg.KEY_FN = {}
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
    sg.FILES[name] = ptn
    sg.KEY_FN[name] = key_fnc
    db = shelve.open(path.join(edr, "{}.db".format(name)))
    sg.plot_seq(sg.get_data(db, name), name, odr, ins, flt, order)
    db.close()


def main():
    # Graphs:
    # - Motivation:
    #   (1) Sequence: Long nights/days, static buffers, CUBIC
    #   (2) Sequence: Short nights/days, static buffers, CUBIC
    #   (3) Sequence: Very short nights/days, static buffers, CUBIC
    #   (4) Sequence: Short nights/days, static buffers, all TCP variants
    # - Contributions:
    #   (5) Static buffers, CUBIC
    #     (5.1) Sequence
    #     (5.2) Utilization
    #     (5.3) Latency 50
    #     (5.4) Latency 99
    #   (6) Dynamic buffers, CUBIC.
    #     (6.1) Sequence
    #     (6.2) Utilization
    #     (6.3) Latency 50
    #     (6.4) Latency 99
    #   (7) Dynamic buffers, all TCP variants
    #     (7.1) Sequence
    #     (7.2) Utilization
    #     (7.3) Latency 50
    #     (7.4) Latency 99
    #   (8) Static buffers, reTCP
    #     (8.1) Sequence
    #     (8.2) Utilization
    #     (8.3) Latency 50
    #     (8.4) Latency 99
    #   (9) Dynamic buffers, reTCP
    #     (9.1) Sequence
    #     (9.2) Utilization
    #     (9.3) Latency 50
    #     (9.4) Latency 99

    edr = sys.argv[1]
    if not path.isdir(edr):
        print("The first argument must be a directory, but is: {}".format(edr))
        sys.exit(-1)
    # Specify and create the output directory.
    odr = path.join(PROGDIR, 'graphs', 'nsdi2020')
    if path.exists(odr):
        if not path.isdir(odr):
            print("Output directory exists and is a file: {}".format(odr))
            sys.exit(-1)
    else:
        os.makedirs(odr)

    # (1)
    seq(name="1_old-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=OLD_PTN.format("cubic"),
        key_fnc=lambda fn: "cubic",
        dur=60000)

    # (2)
    seq(name="current-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format(CHOSEN_STATIC, "cubic"),
        key_fnc=lambda fn: "cubic",
        dur=1200)

    # (3)
    seq(name="future-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=FUTURE_PTN.format("cubic"),
        key_fnc=lambda fn: "cubic",
        dur=6)

    # (4)
    seq(name="current-all",
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format(CHOSEN_STATIC, "*"),
        key_fnc=lambda fn: fn.split("-")[7],
        dur=1200,
        flt=(lambda idx, label, ccs=DESIRED_CCS: label in ccs))

    # (5.1)
    seq(name="static-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format("*", "cubic"),
        key_fnc=lambda fn: fn.split("-")[3],
        dur=1200)

    # (6.1)
    seq(name="dyn-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("*", "cubic"),
        key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                     / python_config.TDF)),
        dur=1200,
        ins=DYN_INS,
        flt=(lambda idx, label, resize_us=DESIRED_RESIZE_US: \
             idx in resize_us),
        order=["optimal", "packet only", "static", "25", "75", "125", "150",
               "175", "225"])

    # (7.1)
    seq(name="dyn-all",
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("3500", "*"),
        key_fnc=lambda fn: fn.split("-")[7],
        dur=1200,
        flt=(lambda idx, label, ccs=DESIRED_CCS: label in ccs))

    # (8.1)
    seq(name="static-retcp",
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format("*", "retcp"),
        key_fnc=lambda fn: fn.split("-")[3],
        dur=1200)

    # (9.1)
    seq(name="dyn-retcp",
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("*", "retcp"),
        key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                     / python_config.TDF)),
        dur=1200,
        flt=(lambda idx, label, resize_us=DESIRED_RESIZE_US: \
             idx in resize_us),
        order=["optimal", "packet only", "static", "25", "75", "125", "150",
               "175", "225"])


if __name__ == "__main__":
    main()
