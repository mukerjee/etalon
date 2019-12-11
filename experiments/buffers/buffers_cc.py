#!/usr/bin/env python

from os import path
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For click_common and common.
sys.path.insert(0, path.join(PROGDIR, ".."))
# For buffer_common.
sys.path.insert(0, path.join(PROGDIR, "..", ".."))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "..", "etc"))

import buffer_common
import click_common
import common
import python_config


# If True, then do not run experiments and instead only print configurations.
DRY_RUN = False
# Flowgrind flow duration, in seconds.
# (week length (in us) x 3 + 100 us (for good measure)) x (1 s / 1000000 us) * 1000
DUR_S = 4.54
# This schedule mimics an 8-rack cluster where each rack gets a circuit to every
# other rack for 180us. Values are the srcs, indices are the dsts. E.g., 1/2/0
# means that 1->0, 2->1, and 0->2.
FIXED = "4 25200 1/2/0 400 -1/-1/-1 3600 2/0/1 400 -1/-1/-1"
# FIXED = None
# Control which experimnets are run.
RESIZE_US_MIN = 100
RESIZE_US_MAX = 100
RESIZE_US_DELTA = 25


def maybe(fnc, do=not DRY_RUN):
    if do:
        fnc()


def main():
    # Generate the list of configurations first so that we know the total number
    # of experiments. CC modes are the outside loop to minimize how frequently
    # we change the CC mode, since doing so requires restarting the cluster.
    ccs = ["cubic"]
    cnfs = []
    for cc in ccs:
        sweep = buffer_common.gen_resize_sweep(
            int(round(RESIZE_US_MIN * python_config.TDF)),
            int(round(RESIZE_US_MAX * python_config.TDF)),
            int(round(RESIZE_US_DELTA * python_config.TDF)))

        for cnf in sweep:
            cnf["cc"] = cc
            if cc == "dctcp":
                # For DCTCP, enable threshold-based ECN marking.
                cnf['ecn'] = python_config.DCTCP_THRESH
            if FIXED is not None:
                cnf["type"] = "fixed"
                cnf["fixed_schedule"] = FIXED
        cnfs += sweep
    # Total number of experiments.
    tot = len(cnfs)

    # Use the first CC mode when starting the cluster to avoid needing to
    # restart the cluster for the first CC mode's experiments.
    maybe(lambda: common.initializeExperiment("flowgrindd", ccs[0]))
    for cnt, cnf in enumerate(cnfs, 1):
        maybe(lambda: click_common.setConfig(cnf))
        print("--- experiment {} of {}, config:\n{}".format(cnt, tot, cnf))
        maybe(lambda: common.flowgrind(
            settings={"flows": [{"src": "r1", "dst": "r2"}], "dur": DUR_S}))
    maybe(lambda: common.finishExperiment())


if __name__ == "__main__":
    main()
