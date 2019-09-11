#!/usr/bin/env python
#
# This program generates graphs for experiments produced by
# etalon/experiments/buffers/nsdi2020.py.

# Uncomment these lines to use LaTeX font for graphs.
# import matplotlib
# matplotlib.rcParams['text.usetex'] = True

import os
from os import path
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "..", "etc"))
# For sg.
sys.path.insert(0, path.join(PROGDIR, "sequence_graphs"))

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
DYN_INS = ((600, 820), (35, 275))
# CC mode indices to display in static graph.
DESIRED_CCS = ["optimal", "packet only", "bbr", "cubic", "dctcp", "highspeed",
               "illinois", "scalable", "westwood", "yeah"]
# Order of the lines for the dynamic buffer resizing experiments. This is also
# used to select which lines to plot.
DYN_ORDER = ["optimal", "packet only", "static", "25", "75", "100", "125",
             "150", "175", "225"]
# The TCP variant to use as our baseline.
CHOSEN_TCP = "cubic"
# Static buffer size to use.
CHOSEN_STATIC = 16


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
    sg.seq(
        name="1_old-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=OLD_PTN.format(CHOSEN_TCP),
        key_fnc=lambda fn, chosen_tcp=CHOSEN_TCP: chosen_tcp,
        dur=60000)

    # (2)
    sg.seq(
        name="2_current-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format(CHOSEN_STATIC, CHOSEN_TCP),
        key_fnc=lambda fn, chosen_tcp=CHOSEN_TCP: chosen_TCP,
        dur=1200)

    # (3)
    sg.seq(
        name="3_future-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=FUTURE_PTN.format(CHOSEN_TCP),
        key_fnc=lambda fn, chosen_tcp=CHOSEN_TCP: chosen_tcp,
        dur=6)

    # (4)
    sg.seq(
        name="4_current-all",
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format(CHOSEN_STATIC, "*"),
        key_fnc=lambda fn: fn.split("-")[7],
        dur=1200,
        flt=lambda idx, label, ccs=DESIRED_CCS: label in ccs)

    # (5.1)
    sg.seq(
        name="5.1_static-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format("*", CHOSEN_TCP),
        key_fnc=lambda fn: fn.split("-")[3],
        dur=1200)

    # (6.1)
    # With and without inset.
    for ins in [DYN_INS, None]:
        sg.seq(
            name="6.1_dyn-{}{}".format(
                CHOSEN_TCP, "_inset" if ins is not None else ""),
            edr=edr,
            odr=odr,
            ptn=DYN_PTN.format("*", CHOSEN_TCP),
            key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                         / python_config.TDF)),
            dur=1200,
            ins=ins,
            flt=(lambda idx, label, order=DYN_ORDER: \
                 label.strip(" $\mu$s") in order),
            order=DYN_ORDER)

    # (7.1)
    sg.seq(
        name="7.1_dyn-all",
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("3500", "*"),
        key_fnc=lambda fn: fn.split("-")[7],
        dur=1200,
        flt=(lambda idx, label, ccs=DESIRED_CCS: label in ccs))

    # (8.1)
    sg.seq(
        name="8.1_static-retcp",
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format("*", "retcp"),
        key_fnc=lambda fn: fn.split("-")[3],
        dur=1200)

    # (9.1)
    sg.seq(
        name="9.1_dyn-retcp",
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("*", "retcp"),
        key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                     / python_config.TDF)),
        dur=1200,
        flt=lambda idx, label, order=DYN_ORDER: label.strip(" $\mu$s") in order,
        order=DYN_ORDER)


if __name__ == "__main__":
    main()
