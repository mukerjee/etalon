#!/usr/bin/env python

from collections import defaultdict
import os
from os import path
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))

from dotmap import DotMap
import simpleplotlib
simpleplotlib.default_options.rcParams['font.family'] = "Tahoma"
from simpleplotlib import plot

MSS_B_EXP = 1500
MSS_B_TARGET = 9000


def main():
    assert len(sys.argv) == 2, "Expected one argument: experiment data file"
    edf = sys.argv[1]
    if not path.isfile(edf):
        print("The first argument must be a file, but is: {}".format(edf))
        sys.exit(-1)
    # Specify and create the output directory.
    odr = path.join(PROGDIR, "graphs", "nsdi2020")
    if path.exists(odr):
        if not path.isdir(odr):
            print("Output directory exists and is a file: {}".format(odr))
            sys.exit(-1)
    else:
        os.makedirs(odr)

    # { num flows : { q len : list of results } }
    data = defaultdict(lambda: defaultdict(dict))
    with open(edf) as f:
        for line in f:
            date, swt_us, num_flows, q_len_p, delay_2way_ns, fct_us, num_rtx, \
                num_rto, num_syns, avt_rtt_us = line.strip().split()
            # Convert the queue length from being in terms of 1500-byte packets
            # to being in terms of 9000-bytes packets.
            q_len_p = float(q_len_p) * (float(MSS_B_EXP) / float(MSS_B_TARGET))
            swt_us = float(swt_us)
            fct_s = float(fct_us) / 1e6
            record = (date, float(delay_2way_ns), fct_s, float(num_rtx),
                      float(num_rto), float(num_syns), float(avt_rtt_us))

            # Store this record if it is the smallest FCT for this q len.
            if ((swt_us in data[num_flows][q_len_p] and
                 fct_s < data[num_flows][q_len_p][swt_us][2]) or
                    swt_us not in data[num_flows][q_len_p]):
                data[num_flows][q_len_p][swt_us] = record


    for num_flows, q_len_results in data.items():
        # for q_len_p, swt_us_results in q_len_results.items():
        #     for swt_us, val in swt_us_results.items():
        #         print val

        # assert False
        # { q len : list of pairs (switch time, FCT) }.items()
        lines = {q_len_p : [
            (swt_us, fct_s)
            for swt_us, (_, _, fct_s, _, _, _, _) in swt_us_results.items()]
                 for q_len_p, swt_us_results in q_len_results.items()}.items()
        # Sort the datapoints based on their x-valies.
        lines = sorted(lines, key=lambda a: a[0])
        # lbls: list of q lens
        # lines: list of lists of pairs of (switch time, FCT)
        lbls, lines = zip(*lines)
        # xs: list of lists of switch times
        # ys: list of lists of FCTs
        xs = [[p[0] for p in sorted(val, key=lambda a: a[0])] for val in lines]
        ys = [[p[1] for p in sorted(val, key=lambda a: a[0])] for val in lines]

        options = DotMap()
        options.plot_type = "LINE"
        options.legend.options.labels = [
            "{} packets".format(int(round(lbl))) for lbl in lbls]
        options.legend.options.fontsize = 18
        options.legend.options.ncol = 1
        options.series_options = [DotMap(linewidth=2) for _ in range(len(xs))]
        options.output_fn = path.join(odr, "sim-{}-flows.pdf".format(num_flows))
        options.x.label.xlabel = "Circuit uptime ($\mu$s)"
        options.y.label.ylabel = "Flow completion time (s)"
        options.x.label.fontsize = options.y.label.fontsize = 18
        options.x.ticks.major.options.labelsize = \
            options.y.ticks.major.options.labelsize = 18
        options.x.log = True
        options.x.axis.show = options.y.axis.show = True
        options.x.axis.color = options.y.axis.color = "black"
        options.y.limits = (0, max([max(line) for line in ys]) + 4)

        plot(xs, ys, options)


if __name__ == "__main__":
    main()
