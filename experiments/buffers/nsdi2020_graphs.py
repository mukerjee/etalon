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

# Use non-interactive backend.
import matplotlib
matplotlib.use("Agg")

import buffers_graphs
import python_config
import sg

# True and False mean that the data parsing will be executed using a single
# thread and multiple threads, respectively.
SYNC = False

# Experiment parameters.
#
# One-wa circuit latency.
CIR_LAT_s = python_config.CIRCUIT_LATENCY_s
# Circuit uptime.
RECNF_us = python_config.RECONFIG_DELAY_us
DAY_LEN_us = 9 * RECNF_us
# The number of racks that the fake strobe schedule emulated.
NUM_RACKS_FAKE = 25
# Calculate experiment duration.
DUR_us = int(round((RECNF_us + DAY_LEN_us) * (NUM_RACKS_FAKE - 1) * 3))
# The location of the HSLog element: either "before" or "after" the hybrid
# switch.
LOG_POS = "after"
# The default length to use when reading individual packet log messages.
DEFAULT_MSG_LEN = 116
# Small static buffer size to use.
CHOSEN_STATIC_SMALL = 16

# Filename patterns.
#
# Matches experiments with static buffers with a particular small capacity, a
# particular CC mode, and days and nights of certain lengths.
STATIC_PTN = "*-{}-*-QUEUE-False-*-{}-*-{}-{}-*-click.txt"
# Matches experiments with static buffers with a particular small capacity, a
# particular CC mode, 20 us nights, and 180 us days (under TDF).
STATIC_PTN_CUR = STATIC_PTN.format("{}", "{}",
                                   int(round(RECNF_us * python_config.TDF)),
                                   int(round(DAY_LEN_us * python_config.TDF)))
# Matches experiments with dynamic buffers, a particular resize time, and a
# particular CC mode.
DYN_PTN = "*-QUEUE-True-{}-{}-*-click.txt"

# Selection and ordering of the lines.
#
# TCP variants.
ORDER_VARS = ["optimal", "bbr", "cubic", "dctcp", "highspeed",
              "illinois", "scalable", "westwood", "yeah", "packet only"]
# Static buffers.
ORDER_STATIC_SEQ = ["optimal", "128", "64", "32", "16", "8", "4", "packet only"]
ORDER_STATIC_UTIL = ["4", "8", "16", "32", "64", "128"]
# Dynamic buffers.
# ORDER_DYN = ["optimal", "175", "150", "125", "100", "75", "50", "25", "0",
#                 "packet only"]
# ORDER_DYN = ["optimal", "1200", "1100", "1000", "800", "600", "400", "200",
#                 "0", "packet only"]
# ORDER_DYN = ["optimal", "600", "500", "400", "300", "200", "100", "0",
#                 "packet only"]
# ORDER_DYN = ["optimal", "1000", "500", "0", "packet only"]
# ORDER_DYN = ["optimal", "1100", "1000", "900", "800", "700", "500", "0",
#                 "packet only"]
# ORDER_DYN = ["optimal", "3000", "2700", "2400", "2100", "1800", "1200", "600", "0",
#                 "packet only"]
ORDER_DYN = ["optimal", "2400", "1800", "600", "0", "packet only"]
# ORDER_DYN = ["optimal", "300", "250", "200", "150", "100", "50", "0",
#                 "packet only"]
# ORDER_DYN = ["optimal", "300", "200", "100", "0", "packet only"]
# ORDER_DYN = ["optimal", "7000", "6000", "5000", "4000", "3000", "2000",
#                 "1000", "0", "packet only"]
# ORDER_DYN = ["optimal", "10000", "8000", "7000", "6000", "5000", "4000",
#                 "2000", "0", "packet only"]
# Dynamic buffers + reTCP.
ORDER_DYN_RETCP = ["optimal", "250", "150", "50", "0", "packet only"]
ORDER_DYN_RETCP_UTIL = [0, 50, 100, 150, 200, 250, 300]
# Dynamic buffer resizing with multiple TCP variants.
CHOSEN_DYN_uss = [1200]
ORDER_DYN_uss = (["optimal"] +
                 list(reversed([str(us) for us in CHOSEN_DYN_uss])) +
                 ["packet only"])
# Dynamic buffer resizing experiments to analyze using chunk mode.
DYNS_TO_EXAMINE = [0, 25, 50, 75, 100, 125, 150, 175, 200, 225]
# DYNS_TO_EXAMINE = [0, 200, 400, 600, 800, 1000, 1200, 1400, 1600, 1800, 2000]

# Other parameters.
#
# The TCP variant to use as our baseline.
CHOSEN_TCP = "cubic"
# Inset window bounds.
DYN_INS = ((600, 820), (35, 275))
# Bounds for zooming in on the graphs.
# XLM_ZOOM = None
XLM_ZOOM = (2200, 5200)
# XLM_ZOOM = (1300, 2900)
# XLM_ZOOM = (9220, 9700)
# XLM_ZOOM = (4000, 8120)
# YLM_ZOOM = None
YLM_ZOOM = (150, 550)
# YLM_ZOOM = (750, 1000)
# YLM_ZOOM = (0, 15000)
XLM_ZOOM_STATIC = (4100, 5150)
YLM_ZOOM_STATIC = (300, 500)
XLM_ZOOM_RETCP = XLM_ZOOM_STATIC
YLM_ZOOM_RETCP = YLM_ZOOM_STATIC


def main():
    num_args = len(sys.argv)
    assert num_args == 2 or num_args == 3, \
        ("Expected either one or two arguments: experiment data directory "
         "[log message size (bytes)]")
    edr = sys.argv[1]
    if not path.isdir(edr):
        print("The first argument must be a directory, but is: {}".format(edr))
        sys.exit(-1)
    # Specify and create the output directory.
    odr = path.join(PROGDIR, "graphs", "nsdi2020")
    if path.exists(odr):
        if not path.isdir(odr):
            print("Output directory exists and is a file: {}".format(odr))
            sys.exit(-1)
    else:
        os.makedirs(odr)
    # Parse the log message length.
    if num_args == 3:
        try:
            msg_len = int(sys.argv[2])
        except ValueError:
            print("Log message length must be an int, but is: {}".format(
                sys.argv[2]))
            sys.exit(-1)
        assert msg_len > 0, \
            ("Log message length must be greater than 0, but is: "
             "{}").format(msg_len)
    else:
        msg_len = DEFAULT_MSG_LEN

    def _1():
        rcf_us = 1000
        day_len_us = 9 * rcf_us
        sg.seq(
            sync=SYNC,
            name="1_seq-old-{}".format(CHOSEN_TCP),
            edr=edr,
            odr=odr,
            # Matches experiments with static buffers with a particular small
            # capacity, a particular CC mode, 1000 us nights, and 9000 us days
            # (under TDF).
            ptn=STATIC_PTN.format(CHOSEN_STATIC_SMALL, CHOSEN_TCP,
                                  int(round(rcf_us * python_config.TDF)),
                                  int(round(day_len_us * python_config.TDF))),
            key_fnc=lambda fn, chosen_tcp=CHOSEN_TCP: chosen_tcp,
            dur=int(round((rcf_us + day_len_us) * (NUM_RACKS_FAKE - 1) * 3)),
            msg_len=msg_len,
            cir_lat_s=CIR_LAT_s,
            log_pos=LOG_POS,
            rcf_us=rcf_us,
            voq_agg=True)

    def _2():
        sg.seq(
            sync=SYNC,
            name="2_seq-current-{}".format(CHOSEN_TCP),
            edr=edr,
            odr=odr,
            ptn=STATIC_PTN_CUR.format(CHOSEN_STATIC_SMALL, CHOSEN_TCP),
            key_fnc=lambda fn, chosen_tcp=CHOSEN_TCP: chosen_tcp,
            dur=DUR_us,
            msg_len=msg_len,
            cir_lat_s=CIR_LAT_s,
            log_pos=LOG_POS,
            voq_agg=True)

    def _3():
        rcf_us = 1
        day_len_us = 9 * rcf_us
        sg.seq(
            sync=SYNC,
            name="3_seq-future-{}".format(CHOSEN_TCP),
            edr=edr,
            odr=odr,
            # Matches experiments with static buffers with a particular small
            # capacity, a particular CC mode, 1 us nights, and 9 us days (under
            # TDF).
            ptn=STATIC_PTN.format(CHOSEN_STATIC_SMALL, CHOSEN_TCP,
                                  int(round(rcf_us * python_config.TDF)),
                                  int(round(day_len_us * python_config.TDF))),
            key_fnc=lambda fn, chosen_tcp=CHOSEN_TCP: chosen_tcp,
            dur=int(round((rcf_us + day_len_us) * (NUM_RACKS_FAKE - 1) * 3)),
            msg_len=msg_len,
            cir_lat_s=CIR_LAT_s,
            log_pos=LOG_POS,
            rcf_us=rcf_us,
            voq_agg=True)

    def _4_1():
        sg.seq(
            sync=SYNC,
            name="4-1_seq-current-all",
            edr=edr,
            odr=odr,
            ptn=STATIC_PTN_CUR.format(CHOSEN_STATIC_SMALL, "*"),
            key_fnc=lambda fn: fn.split("-")[8],
            dur=DUR_us,
            flt=lambda idx, label, ccs=ORDER_VARS: label in ccs,
            order=ORDER_VARS,
            msg_len=msg_len,
            cir_lat_s=CIR_LAT_s,
            log_pos=LOG_POS,
            xlm=XLM_ZOOM_STATIC,
            ylm=YLM_ZOOM_STATIC)

    def _4_2():
        buffers_graphs.util(
            sync=SYNC,
            name="4-2_util-lat-current-all_util",
            edr=edr,
            odr=odr,
            ptn=STATIC_PTN_CUR.format(CHOSEN_STATIC_SMALL, "*"),
            key_fnc=lambda fn: fn.split("-")[8],
            xlb="TCP variant",
            srt=False,
            xlr=45,
            lbs=12,
            flt=lambda key: key != "retcp",
            num_racks=NUM_RACKS_FAKE,
            msg_len=msg_len)

    def _5_1():
        sg.seq(
            sync=SYNC,
            name="5-1_seq-static-{}".format(CHOSEN_TCP),
            edr=edr,
            odr=odr,
            ptn=STATIC_PTN_CUR.format("*", CHOSEN_TCP),
            key_fnc=lambda fn: fn.split("-")[3],
            dur=DUR_us,
            order=ORDER_STATIC_SEQ,
            msg_len=msg_len,
            cir_lat_s=CIR_LAT_s,
            log_pos=LOG_POS,
            voq_agg=True,
            xlm=XLM_ZOOM_STATIC,
            ylm=YLM_ZOOM_STATIC)

    def _5_2():
        buffers_graphs.util(
            sync=SYNC,
            name="5-2_util-lat-static-{}_util".format(CHOSEN_TCP),
            edr=edr,
            odr=odr,
            ptn=STATIC_PTN_CUR.format("*", CHOSEN_TCP),
            key_fnc=lambda fn: fn.split("-")[3],
            xlb="Static buffer size (packets)",
            order=ORDER_STATIC_UTIL,
            num_racks=NUM_RACKS_FAKE,
            msg_len=msg_len)

    def _5_3():
        buffers_graphs.lat(
            sync=SYNC,
            name="5-3_util-lat-static-{}_lat50".format(CHOSEN_TCP),
            edr=edr,
            odr=odr,
            ptn=STATIC_PTN_CUR.format("*", CHOSEN_TCP),
            key_fnc=lambda fn: fn.split("-")[3],
            prc=50,
            ylb="Median",
            msg_len=msg_len)

    def _5_4():
        buffers_graphs.lat(
            sync=SYNC,
            name="5-4_util-lat-static-{}_lat99".format(CHOSEN_TCP),
            edr=edr,
            odr=odr,
            ptn=STATIC_PTN_CUR.format("*", CHOSEN_TCP),
            key_fnc=lambda fn: fn.split("-")[3],
            prc=99,
            ylb="99th percentile",
            msg_len=msg_len)

    def _6_1_1():
        # With and without inset.
        for ins in [None]:
            sg.seq(
                sync=SYNC,
                name="6-1-1_seq-dyn-{}{}".format(
                    CHOSEN_TCP,
                    "_inset" if ins is not None else ""),
                edr=edr,
                odr=odr,
                ptn=DYN_PTN.format("*", CHOSEN_TCP),
                key_fnc=lambda fn: int(round(float(fn.split("-")[7]) /
                                             python_config.TDF)),
                dur=DUR_us,
                ins=ins,
                flt=(lambda idx, label, order=ORDER_DYN: \
                     label.strip(" $\mu$s") in order),
                order=ORDER_DYN,
                msg_len=msg_len,
                voq_agg=True,
                cir_lat_s=CIR_LAT_s,
                log_pos=LOG_POS,
                xlm=XLM_ZOOM,
                ylm=YLM_ZOOM)

    def _6_1_2():
        for dyn_us in DYNS_TO_EXAMINE:
            # With and without zooming in.
            for xlm_zoom, ylm_zoom in [(XLM_ZOOM, YLM_ZOOM), (None, None)]:
                sg.seq(
                    sync=SYNC,
                    name="6-1-3_seq-dyn-{}_{}_chunk{}".format(
                        CHOSEN_TCP, dyn_us, "" \
                        if xlm_zoom is None else "_zoom"),
                    edr=edr,
                    odr=odr,
                    ptn=DYN_PTN.format(
                        int(round(dyn_us * python_config.TDF)), CHOSEN_TCP),
                    key_fnc=lambda fn: int(round(float(fn.split("-")[7])
                                                 / python_config.TDF)),
                    dur=DUR_us,
                    flt=None,
                    xlm=xlm_zoom,
                    ylm=ylm_zoom,
                    chunk_mode=500,
                    msg_len=msg_len,
                    cir_lat_s=CIR_LAT_s,
                    log_pos=LOG_POS)

    def _6_2():
        buffers_graphs.util(
            sync=SYNC,
            name="6-2_util-lat-dyn-{}_util".format(CHOSEN_TCP),
            edr=edr,
            odr=odr,
            ptn=DYN_PTN.format("*", CHOSEN_TCP),
            key_fnc=lambda fn: int(round(float(fn.split("-")[7])
                                         / python_config.TDF)),
            xlb="Resize time ($\mu$s)",
            xlr=45,
            lbs=12,
            num_racks=NUM_RACKS_FAKE,
            msg_len=msg_len)

    def _6_3():
        buffers_graphs.lat(
            sync=SYNC,
            name="6-3_util-lat-dyn-{}_lat50".format(CHOSEN_TCP),
            edr=edr,
            odr=odr,
            ptn=DYN_PTN.format("*", CHOSEN_TCP),
            key_fnc=lambda fn: int(round(float(fn.split("-")[7])
                                         / python_config.TDF)),
            prc=50,
            ylb="Median",
            ylm=500,
            xlr=45,
            msg_len=msg_len)

    def _6_4():
        buffers_graphs.lat(
            sync=SYNC,
            name="6-4_util-lat-dyn-{}_lat99".format(CHOSEN_TCP),
            edr=edr,
            odr=odr,
            ptn=DYN_PTN.format("*", CHOSEN_TCP),
            key_fnc=lambda fn: int(round(float(fn.split("-")[7])
                                         / python_config.TDF)),
            prc=99,
            ylb="99th percentile\n",
            ylm=500,
            xlr=45,
            msg_len=msg_len)

    def _7_1_1_and_7_2():
        for us in CHOSEN_DYN_uss:
            us_tdf = int(round(us * python_config.TDF))
            sg.seq(
                sync=SYNC,
                name="7-1-1_seq-dyn-all-{}us".format(us),
                edr=edr,
                odr=odr,
                ptn=DYN_PTN.format(us_tdf, "*"),
                key_fnc=lambda fn: fn.split("-")[8],
                dur=DUR_us,
                flt=lambda idx, label, ccs=ORDER_VARS: label in ccs,
                order=ORDER_VARS,
                msg_len=msg_len,
                cir_lat_s=CIR_LAT_s,
                log_pos=LOG_POS)
            buffers_graphs.util(
                sync=SYNC,
                name="7-2_util-lat-dyn-all-{}us_util".format(us),
                edr=edr,
                odr=odr,
                ptn=DYN_PTN.format(us_tdf, "*"),
                key_fnc=lambda fn: fn.split("-")[8],
                xlb="TCP variant",
                srt=False,
                xlr=45,
                lbs=12,
                flt=lambda key: key != "retcp",
                num_racks=NUM_RACKS_FAKE,
                msg_len=msg_len)

    def _7_1_2():
        for cc in python_config.CCS:
            sg.seq(
                sync=SYNC,
                name="7-1-2_seq-dyn-{}".format(cc),
                edr=edr,
                odr=odr,
                ptn=DYN_PTN.format("*", cc),
                key_fnc=lambda fn: int(round(float(fn.split("-")[7])
                                             / python_config.TDF)),
                dur=DUR_us,
                flt=(lambda idx, label, order=ORDER_DYN_uss: \
                     label.strip(" $\mu$s") in order),
                order=ORDER_DYN_uss,
                msg_len=msg_len,
                cir_lat_s=CIR_LAT_s,
                log_pos=LOG_POS)

    def _8_1():
        sg.seq(
            sync=SYNC,
            name="8-1_seq-static-retcp",
            edr=edr,
            odr=odr,
            ptn=STATIC_PTN_CUR.format("*", "retcp"),
            key_fnc=lambda fn: fn.split("-")[3],
            order=ORDER_STATIC_SEQ,
            dur=DUR_us,
            msg_len=msg_len,
            cir_lat_s=CIR_LAT_s,
            log_pos=LOG_POS,
            voq_agg=True,
            xlm=XLM_ZOOM_RETCP,
            ylm=YLM_ZOOM_RETCP)

    def _8_2():
        buffers_graphs.util(
            sync=SYNC,
            name="8-2_util-lat-static-retcp_util",
            edr=edr,
            odr=odr,
            ptn=STATIC_PTN_CUR.format("*", "retcp"),
            key_fnc=lambda fn: fn.split("-")[3],
            order=ORDER_STATIC_UTIL,
            xlb="Static buffer size (packets)",
            num_racks=NUM_RACKS_FAKE,
            msg_len=msg_len)

    def _8_3():
        buffers_graphs.lat(
            sync=SYNC,
            name="8-3_util-lat-static-retcp_lat50",
            edr=edr,
            odr=odr,
            ptn=STATIC_PTN_CUR.format("*", "retcp"),
            key_fnc=lambda fn: fn.split("-")[3],
            prc=50,
            ylb="Median",
            msg_len=msg_len)

    def _8_4():
        buffers_graphs.lat(
            sync=SYNC,
            name="8-4_util-lat-static-retcp_lat99",
            edr=edr,
            odr=odr,
            ptn=STATIC_PTN_CUR.format("*", "retcp"),
            key_fnc=lambda fn: fn.split("-")[3],
            prc=99,
            ylb="99th percentile",
            msg_len=msg_len)

    def _9_1():
        sg.seq(
            sync=SYNC,
            name="9-1_seq-dyn-retcp",
            edr=edr,
            odr=odr,
            ptn=DYN_PTN.format("*", "retcp"),
            key_fnc=lambda fn: int(round(float(fn.split("-")[7])
                                         / python_config.TDF)),
            dur=DUR_us,
            flt=(lambda idx, label, order=ORDER_DYN_RETCP:
                 label.strip(" $\mu$s") in order),
            order=ORDER_DYN_RETCP,
            cir_lat_s=CIR_LAT_s,
            log_pos=LOG_POS,
            msg_len=msg_len,
            voq_agg=True,
            xlm=XLM_ZOOM_RETCP,
            ylm=YLM_ZOOM_RETCP)

    def _9_2():
        buffers_graphs.util(
            sync=SYNC,
            name="9-2_util-lat-dyn-retcp_util",
            edr=edr,
            odr=odr,
            ptn=DYN_PTN.format("*", "retcp"),
            key_fnc=lambda fn: int(round(float(fn.split("-")[7])
                                         / python_config.TDF)),
            flt=lambda key, order=ORDER_DYN_RETCP_UTIL: key in order,
            order=ORDER_DYN_RETCP_UTIL,
            xlb="Resize time ($\mu$s)",
            xlr=45,
            lbs=12,
            num_racks=NUM_RACKS_FAKE,
            msg_len=msg_len)

    def _9_3():
        buffers_graphs.lat(
            sync=SYNC,
            name="9-3_util-lat-dyn-retcp_lat50",
            edr=edr,
            odr=odr,
            ptn=DYN_PTN.format("*", "retcp"),
            key_fnc=lambda fn: int(round(float(fn.split("-")[7])
                                         / python_config.TDF)),
            flt=lambda key, order=ORDER_DYN_RETCP_UTIL: key in order,
            prc=50,
            ylb="Median",
            msg_len=msg_len)

    def _9_4():
        buffers_graphs.lat(
            sync=SYNC,
            name="9-4_util-lat-dyn-retcp_lat99",
            edr=edr,
            odr=odr,
            ptn=DYN_PTN.format("*", "retcp"),
            key_fnc=lambda fn: int(round(float(fn.split("-")[7])
                                         / python_config.TDF)),
            flt=lambda key, order=ORDER_DYN_RETCP_UTIL: key in order,
            prc=99,
            ylb="99th percentile",
            msg_len=msg_len)

    def _4():
        _4_1()
        _4_2()

    def _5():
        _5_1()
        _5_2()
        _5_3()
        _5_4()

    def _6():
        _6_1_1()
        # _6_1_2()
        _6_2()
        _6_3()
        _6_4()

    def _7():
        _7_1_1_and_7_2()
        _7_1_2()

    def _8():
        _8_1()
        _8_2()
        _8_3()
        _8_4()

    def _9():
        _9_1()
        _9_2()
        _9_3()
        _9_4()

    # Note: The numbers below have no correlation with the sections or figures
    #       in the paper.
    #
    # Graphs:
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
    #       (6.1.2) For one experiment, look at all flows in detail
    #     (6.2) Utilization
    #     (6.3) Latency 50
    #     (6.4) Latency 99
    #   (7) Dynamic buffers, all TCP variants
    #     (7.1) Sequence
    #       (7.1.1) Fixed resize time, varying variant
    #       (7.1.2) Fixed variant, varying resize time
    #     (7.2) Utilization, for various resize times
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

    # _1()
    # _2()
    # _3()
    _4()
    _5()
    _6()
    _7()
    _8()
    _9()


if __name__ == "__main__":
    main()
