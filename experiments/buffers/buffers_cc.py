#!/usr/bin/env python

import copy
import sys
sys.path.insert(0, "/etalon/experiments")

import buffer_common
import click_common
import common

# All available CC modules. Found by:
#     sudo sysctl net.ipv4.tcp_available_congestion_control
CCMS = ["reno", "cubic", "retcp", "dctcp", "bbr", "bic", "cdg", "highspeed",
        "htcp", "hybla", "illinois", "lp", "nv", "scalable", "vegas", "veno",
        "westwood", "yeah"]


def main():
    common.initializeExperiment("flowgrindd")

    # Skip the reTCP experiments because they will be run anyway when
    # cc == "retcp".
    cnfs = [cnf for cnf in buffer_common.CONFIGS if "cc" not in cnf]
    # Total number of experiments.
    tot = len(CCMS) * len(cnfs)
    cnt = 0
    for ccm in CCMS:
        for cnf in cnfs:
            cnt += 1

            # Make a copy to avoid modifying the original configuration object.
            cnf_c = copy.deepcopy(cnf)
            cnf_c["cc"] = ccm
            print("--- running test type {}...".format(cnf_c["type"]))
            print("--- using CC mode {}...".format(cnf_c["cc"]))
            print("--- setting switch buffer size to {}...".format(cnf_c["buffer_size"]))
            click_common.setConfig(cnf_c)
            print("--- done...")
            print("--- experiment {} of {}".format(cnt, tot))
            common.flowgrind(settings={"flows": [{"src": "r1", "dst": "r2"}]})

    common.finishExperiment()


if __name__ == "__main__":
    main()
