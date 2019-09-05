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
# The upper bound on the resize time sweep.
MAX_RESIZE_US = 0
# Flowgrind flow duration, in seconds.
DUR_S = 7
# Fixed schedule: 20 us circuit night, 2000 us pause, 20 us circuit night,
# 180 us circuit day.
FIXED = "2 40800 -1/-1/-1 3600 1/2/0"


def maybe(fnc, do=not DRY_RUN):
    if do:
        fnc()


def main():
    # Generate the list of configurations first so that we know the total number
    # of experiments. CC modes are the outside loop to minimize how frequently
    # we change the CC mode, since doing so requires restarting the cluster.
    ccs = ["cubic", "dctcp", "bbr", "westwood"]
    cnfs = []
    for cc in ccs:
        sweep = buffer_common.gen_resize_sweep(0, MAX_RESIZE_US, 500)
        for cnf in sweep:
            cnf["cc"] = cc
            if cc == "dctcp":
                # For DCTCP, enable threshold-based ECN marking.
                cnf['ecn'] = python_config.DCTCP_THRESH
            if cnf["type"] == "strobe":
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
