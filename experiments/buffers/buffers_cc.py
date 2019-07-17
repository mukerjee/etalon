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


def main():
    common.initializeExperiment("flowgrindd")

    # Skip the reTCP experiments because they will be run anyway when
    # cc == "retcp".
    cnfs = [cnf for cnf in buffer_common.CONFIGS if "cc" not in cnf]
    # Total number of experiments.
    tot = len(python_config.CCMS) * len(cnfs)
    cnt = 0
    # CCMS are the outside loop to minimize how frequently we change the CC
    # mode, since doing so requires restarting the cluster.
    for ccm in python_config.CCMS:
        for cnf in cnfs:
            cnt += 1

            # Make a copy to avoid modifying the original configuration object.
            cnf_c = copy.deepcopy(cnf)
            cnf_c["cc"] = ccm
            print("--- running test type {}...".format(cnf_c["type"]))
            print("--- using CC mode {}...".format(cnf_c["cc"]))
            print("--- setting switch buffer size to {}...".format(
                cnf_c["buffer_size"]))
            click_common.setConfig(cnf_c)
            print("--- done...")
            print("--- experiment {} of {}".format(cnt, tot))
            common.flowgrind(settings={"flows": [{"src": "r1", "dst": "r2"}]})

    common.finishExperiment()


if __name__ == "__main__":
    main()
