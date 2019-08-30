#!/usr/bin/env python

import copy
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


# Flowgrind flow duration, in seconds.
DUR_S = 5
# Fixed schedule: 2000 us pause, 20 us circuit night, 180 us circuit day.
FIXED = "2 40400 -1/-1/-1 3600 1/2/0"


def main():
    ccs = python_config.CCS
    ccs = ccs[:1]

    # Use the first CC mode when starting the cluster to avoid needing to
    # restart the cluster for the first CC mode's experiments.
    common.initializeExperiment("flowgrindd", ccs[0])

    cnfs = buffer_common.CONFIGS
    # Total number of experiments.
    tot = len(ccs) * len(cnfs)
    cnt = 0
    # CC modes are the outside loop to minimize how frequently we change the CC
    # mode, since doing so requires restarting the cluster.
    for cc in ccs:
        for cnf in cnfs:
            cnt += 1

            # Make a copy to avoid modifying the original configuration object.
            cnf_c = copy.deepcopy(cnf)
            cnf_c["cc"] = cc
            if cc == "dctcp":
                # For DCTCP, enable threshold-based ECN marking.
                cnf_c['ecn'] = python_config.DCTCP_THRESH
            if cnf_c["type"] == "strobe":
                cnf_c["type"] = "fixed"
                cnf_c["fixed_schedule"] = FIXED

            print("--- running test type {}...".format(cnf_c["type"]))
            print("--- using CC mode {}...".format(cnf_c["cc"]))
            print("--- setting switch buffer size to {}...".format(
                cnf_c["buffer_size"]))
            click_common.setConfig(cnf_c)
            print("--- done...")
            print("--- experiment {} of {}".format(cnt, tot))
            common.flowgrind(
                settings={"flows": [{"src": "r1", "dst": "r2"}], "dur": DUR_S})

    common.finishExperiment()


if __name__ == "__main__":
    main()
