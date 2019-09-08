#!/usr/bin/env python

from os import path
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For click_common and common.
sys.path.insert(0, path.join(PROGDIR, ".."))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "..", "etc"))

import buffer_common
import click_common
import common
import python_config

# If True, then do not run experiments and instead only print configurations.
DRY_RUN = False
# Flowgrind flow duration, in seconds.
DUR_S = 1.2
# Run static buffer experiments up to buffer size 2**MAX_STATIC_POW.
MAX_STATIC_POW = 7
# Run buffer resizing experiments up to MAX_RESIZE_US us in advance of circuit
# start.
MAX_RESIZE_US = 4500


def maybe(fnc, do=not DRY_RUN):
    """ Executes "fnc" if "do" is True, otherwise does nothing. """
    if do:
        fnc()


def main():
    # Experiments:
    # (1) Long nights/days, static buffers. CUBIC. Old optical switches.
    # (2) Very short nights/days, static buffers, CUBIC. Future optical
    #     switches.
    # (3) Short nights/days, static buffers, all TCP variants.
    # (4) Short nights/days, dynamic buffers, all TCP variants.

    # Assemble configurations. Generate the list of configurations first so that
    # we know the total number of experiments.
    cnfs = []
    # CC modes are the outside loop to minimize how frequently we change the CC
    # mode, since doing so requires restarting the cluster.
    for cc in python_config.CCS:
        if cc in ["cubic"]:
            # (1)
            cnfs += [{"type": "strobe", "buffer_size": 16,
                      "night_len_us": 1000. * python_config.TDF,
                      "day_len_us": 9000. * python_config.TDF,
                      "cc": cc}]
            # (2)
            cnfs += [{"type": "strobe", "buffer_size": 16,
                      "night_len_us": 0.1 * python_config.TDF,
                      "day_len_us": 0.9 * python_config.TDF, "cc": cc}]
        # (3) and (4)
        cnfs += [dict(cnf, cc=cc)
                 for cnf in (
                     buffer_common.gen_static_sweep(mn=2, mx=MAX_STATIC_POW) +
                     buffer_common.gen_resize_sweep(
                         mn=0, mx=MAX_RESIZE_US, dl=500))]
    # Set paramters that apply to all configurations.
    for cnf in cnfs:
        if cnf["cc"] == "dctcp":
            cnf["ecn"] = python_config.DCTCP_THRESH
        cnf["packet_log"] = True

    # Run experiments. Use the first experiment's CC mode to avoid unnecessarily
    # restarting the cluster.
    maybe(lambda: common.initializeExperiment("flowgrindd", cnfs[0]["cc"]))
    # Total number of experiments.
    tot = len(cnfs)
    for cnt, cnf in enumerate(cnfs, start=1):
        maybe(lambda: click_common.setConfig(cnf))
        print("--- experiment {} of {}, config:\n{}".format(cnt, tot, cnf))
        # Generate a flow from each machine on rack 1 to its corresponding
        # partner machine on rack 2.
        maybe(lambda: common.flowgrind(
            settings={"flows": [{"src": "r1", "dst": "r2"}], "dur": DUR_S}))
    maybe(common.finishExperiment)


if __name__ == "__main__":
    main()

    # Graphs
    #
    # Motivation
    # - Sequence: Long nights/days, static buffers, CUBIC. Everything is fine.
    # - Sequence: Short nights/days, static buffers, CUBIC. Cannot use all the
    #   bandwidth.
    # - Sequence: Very short nights/days, static buffers, CUBIC. Everything is
    #   fine?
    # - Sequence: Short nights/days, static buffers, all TCP variants. Still
    #   cannot use all the bandwidth.
    #
    # Contributions
    # - Sequence, utilization, latency 50, latency 99: Static buffers, CUBIC.
    #   Cannot get high utilization and low latency.
    # - Sequence, utilization, latency 50, latency 99: Dynamic buffers, CUBIC.
    # - Sequence, utilization, latency 50, latency 99: Static buffers, reTCP.
    # - Sequence, utilization, latency 50, latency 99: Dynamic buffers, reTCP.
