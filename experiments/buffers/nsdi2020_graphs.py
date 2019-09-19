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

import buffers_graphs
import python_config
import sg

# Filename patterns.
#
# Matches experiments with a particular CC mode, 1000 us nights, and 9000 us
# days (under TDF).
OLD_PTN = "*-{}-*-20000-180000-click.txt"
# Matches experiments with a particular CC mode, 0.1 us nights, and 0.9 us days
# (under TDF).
FUTURE_PTN = "*-{}-*-20-180-click.txt"
# Matches experiments with static buffers of a particular size, a particular CC
# mode, 20 us nights, and 180 us days (under TDF).
STATIC_PTN = "*-{}-QUEUE-False-*-{}-*-400-3600-click.txt"
# Matches experiments with dynamic buffers, a particular resize time, and a
# particular CC mode.
DYN_PTN = "*-QUEUE-True-{}-{}-*click.txt"

# Inset window bounds.
DYN_INS = ((600, 820), (35, 275))
# Order of the lines for the all TCP variants experiments. This is also used to
# select which lines to plot.
ORDER_VARS = ["optimal", "bbr", "cubic", "dctcp", "highspeed",
              "illinois", "scalable", "westwood", "yeah", "packet only"]
# Order of the lines for the static buffers experiments.
ORDER_STATIC = ["optimal", "128", "64", "32", "16", "8", "4", "packet only"]
# Order of the lines for the dynamic buffer resizing experiments. This is also
# used to select which lines to plot. For coarse-grained experiments.
ORDER_DYN_CG = ["optimal", "200", "175", "150", "125", "100", "75", "50", "25",
                "packet only"]
# Same as above. For fine-grained experiments.
ORDER_DYN_FG = ["optimal", "150", "154", "158", "162", "166", "170", "174",
                "packet only"]
# The TCP variant to use as our baseline.
CHOSEN_TCP = "cubic"
# Static buffer size to use.
CHOSEN_STATIC = 16
# Amount of resizing to use.
CHOSEN_RESIZE_US = "3500"
# The x-axis bounds to zoom in on for analyzing circuit teardown.
XLM_ZOOM = (790, 820)
# The y-axis bounds to zoom in on for analyzing circuit teardown.
YLM_ZOOM = (150, 200)
# The chunk to analyze.
CHUNK_IDX = 1000


def main():
    # Note: The numbers below have no correlation with the sections in the
    #       paper.
    #
    # Graphs:
    # - Motivation:
    #   (1) Sequence: Long nights/days, static buffers, CUBIC
    #   (2) Sequence: Short nights/days, static buffers, CUBIC
    #   (3) Sequence: Very short nights/days, static buffers, CUBIC
    #   (4) Short nights/days, static buffers, all TCP variants
    #     (4.1) Sequence
    #     (4.2) Utilization
    # - Contributions:
    #   (5) Static buffers, CUBIC
    #     (5.1) Sequence
    #     (5.2) Utilization
    #     (5.3) Latency 50
    #     (5.4) Latency 99
    #   (6) Dynamic buffers, CUBIC
    #     (6.1) Sequence
    #       (6.1.1) Coarse-grained
    #       (6.1.2) Fine-grained
    #     (6.2) Utilization
    #     (6.3) Latency 50
    #     (6.4) Latency 99
    #   (7) Dynamic buffers, all TCP variants
    #     (7.1.1) Sequence, fixed resize time, varying variant
    #     (7.1.2) Sequence, fixed variant, varying resize time
    #     (7.2) Utilization
    #   (8) Static buffers, reTCP
    #     (8.1) Sequence
    #     (8.2) Utilization
    #     (8.3) Latency 50
    #     (8.4) Latency 99
    #   (9) Dynamic buffers, reTCP
    #     (9.1) Sequence
    #       (9.1.1) Coarse-grained
    #       (9.1.2) Fine-grained
    #     (9.2) Utilization
    #     (9.3) Latency 50
    #     (9.4) Latency 99

    assert len(sys.argv) == 2, \
        "Expected one argument: experiment data directory"
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
        name="1_seq-old-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=OLD_PTN.format(CHOSEN_TCP),
        key_fnc=lambda fn, chosen_tcp=CHOSEN_TCP: chosen_tcp,
        dur=57590)

    # (2)
    sg.seq(
        name="2_seq-current-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format(CHOSEN_STATIC, CHOSEN_TCP),
        key_fnc=lambda fn, chosen_tcp=CHOSEN_TCP: chosen_tcp,
        dur=1200)

    # (3)
    sg.seq(
        name="3_seq-future-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=FUTURE_PTN.format(CHOSEN_TCP),
        key_fnc=lambda fn, chosen_tcp=CHOSEN_TCP: chosen_tcp,
        dur=64)

    # (4.1)
    sg.seq(
        name="4-1_seq-current-all",
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format(CHOSEN_STATIC, "*"),
        key_fnc=lambda fn: fn.split("-")[7],
        dur=1200,
        flt=lambda idx, label, ccs=ORDER_VARS: label in ccs,
        order=ORDER_VARS)

    # (4.2)
    buffers_graphs.util(
        name="4-2_util-current-all",
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format(CHOSEN_STATIC, "*"),
        key_fnc=lambda fn: fn.split("-")[7],
        xlb='TCP variant',
        srt=False,
        xlr=45,
        lbs=12,
        flt=lambda key: key != "retcp")

    # (5.1)
    sg.seq(
        name="5-1_seq-static-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format("*", CHOSEN_TCP),
        key_fnc=lambda fn: fn.split("-")[3],
        dur=1200,
        order=ORDER_STATIC)

    # (5.2)
    buffers_graphs.util(
        name="5-2_util-static-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format("*", CHOSEN_TCP),
        key_fnc=lambda fn: fn.split("-")[3],
        xlb='Static buffer size (packets)')

    # (5.3)
    buffers_graphs.lat(
        name="5-3_lat-50-static-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format("*", CHOSEN_TCP),
        key_fnc=lambda fn: fn.split("-")[3],
        prc=50,
        ylb="Median")

    # (5.4)
    buffers_graphs.lat(
        name="5-4_lat-99-static-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format("*", CHOSEN_TCP),
        key_fnc=lambda fn: fn.split("-")[3],
        prc=99,
        ylb="99th percentile")

    # (6.1.1) With and without inset.
    for ins in [DYN_INS, None]:
        # With a single flow chunk and in aggregate.
        for chunk_idx in [CHUNK_IDX, None]:
            # With and without zooming in.
            for xlm_zoom, ylm_zoom in [(XLM_ZOOM, YLM_ZOOM), (None, None)]:
                plt_typs = ["LINE"]
                if chunk_idx is not None:
                    # Only make scatter plots for the single-flow graphs.
                    plt_typs.append("SCATTER")
                # As line and (optionally) scatter plots.
                for plt_typ in plt_typs:
                    if ins is None or xlm_zoom is None:
                        sg.seq(
                            name="6-1-1_seq-dyn-{}{}{}{}_{}_cg".format(
                                CHOSEN_TCP,
                                ("-chunk{}".format(chunk_idx)
                                 if chunk_idx is not None else ""),
                                "_inset" if ins is not None else "",
                                "_zoom" if xlm_zoom is not None else "",
                                plt_typ),
                            edr=edr,
                            odr=odr,
                            ptn=DYN_PTN.format("*", CHOSEN_TCP),
                            key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                                         / python_config.TDF)),
                            dur=1200,
                            ins=ins,
                            flt=(lambda idx, label, order=ORDER_DYN_CG: \
                                 label.strip(" $\mu$s") in order),
                            order=ORDER_DYN_CG,
                            xlm=xlm_zoom,
                            ylm=ylm_zoom,
                            chunk_idx=chunk_idx,
                            plt_typ=plt_typ)

    # (6.1.2)
    sg.seq(
        name="6-1-2_seq-dyn-{}_fg".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("*", CHOSEN_TCP),
        key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                     / python_config.TDF)),
        dur=1200,
        flt=(lambda idx, label, order=ORDER_DYN_FG: \
             label.strip(" $\mu$s") in order),
        order=ORDER_DYN_FG)

    # (6.2)
    buffers_graphs.util(
        name="6-2_util-dyn-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("*", CHOSEN_TCP),
        key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                     / python_config.TDF)),
        xlb='Resize time ($\mu$s)')

    # (6.3)
    buffers_graphs.lat(
        name="6-3_lat-50-dyn-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("*", CHOSEN_TCP),
        key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                     / python_config.TDF)),
        prc=50,
        ylb="Median")

    # (6.4)
    buffers_graphs.lat(
        name="6-4_lat-99-dyn-{}".format(CHOSEN_TCP),
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("*", CHOSEN_TCP),
        key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                     / python_config.TDF)),
        prc=99,
        ylb="99th percentile")

    # (7.1.1)
    for us in [50, 100, 125, 150, 175]:
        sg.seq(
            name="7-1_seq-dyn-all-{}us".format(
                int(round(us * python_config.TDF))),
            edr=edr,
            odr=odr,
            ptn=DYN_PTN.format(us, "*"),
            key_fnc=lambda fn: fn.split("-")[7],
            dur=1200,
            flt=lambda idx, label, ccs=ORDER_VARS: label in ccs,
            order=ORDER_VARS)

    # (7.1.2)
    for cc in python_config.CCS:
        sg.seq(
            name="7-1_seq-dyn-{}".format(cc),
            edr=edr,
            odr=odr,
            ptn=DYN_PTN.format("*", cc),
            key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                         / python_config.TDF)),
            dur=1200)

    # (7.2)
    buffers_graphs.util(
        name="7-2_util-dyn-all",
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format(CHOSEN_RESIZE_US, "*"),
        key_fnc=lambda fn: fn.split("-")[7],
        xlb='TCP variant',
        srt=False,
        xlr=45,
        lbs=12,
        flt=lambda key: key != "retcp")

    # (8.1)
    sg.seq(
        name="8-1_seq-static-retcp",
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format("*", "retcp"),
        key_fnc=lambda fn: fn.split("-")[3],
        dur=1200)

    # (8.2)
    buffers_graphs.util(
        name="8-2_util-static-retcp",
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format("*", "retcp"),
        key_fnc=lambda fn: fn.split("-")[3],
        xlb='Static buffer size (packets)')

    # (8.3)
    buffers_graphs.lat(
        name="8-3_lat-50-static-retcp",
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format("*", "retcp"),
        key_fnc=lambda fn: fn.split("-")[3],
        prc=50,
        ylb="Median")

    # (8.4)
    buffers_graphs.lat(
        name="8-4_lat-99-static-retcp",
        edr=edr,
        odr=odr,
        ptn=STATIC_PTN.format("*", "retcp"),
        key_fnc=lambda fn: fn.split("-")[3],
        prc=99,
        ylb="99th percentile")

    # (9.1.1)
    sg.seq(
        name="9-1_seq-dyn-retcp",
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("*", "retcp"),
        key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                     / python_config.TDF)),
        dur=1200,
        flt=lambda idx, label, order=ORDER_DYN_CG: label.strip(" $\mu$s") in order,
        order=ORDER_DYN_CG)

    # (9.1.2)
    sg.seq(
        name="6-1_seq-dyn-retcp_fg",
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("*", "retcp"),
        key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                     / python_config.TDF)),
        dur=1200,
        flt=(lambda idx, label, order=ORDER_DYN_FG: \
             label.strip(" $\mu$s") in order),
        order=ORDER_DYN_FG)

    # (9.2)
    buffers_graphs.util(
        name="9-2_util-dyn-retcp",
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("*", "retcp"),
        key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                     / python_config.TDF)),
        xlb='Resize time ($\mu$s)')

    # (9.3)
    buffers_graphs.lat(
        name="9-3_lat-50-dyn-retcp",
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("*", "retcp"),
        key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                     / python_config.TDF)),
        prc=50,
        ylb="Median")

    # (9.4)
    buffers_graphs.lat(
        name="9-4_lat-99-dyn-retcp",
        edr=edr,
        odr=odr,
        ptn=DYN_PTN.format("*", "retcp"),
        key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                     / python_config.TDF)),
        prc=99,
        ylb="99th percentile")


if __name__ == "__main__":
    main()
