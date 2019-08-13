#!/usr/bin/env python

from os import path
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For click_common and common.
sys.path.insert(0, path.join(PROGDIR, ".."))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "..", "etc"))

import click_common
import common
import python_config

# If True, then do not run experiments and instead only print configurations.
DRY_RUN = False
MAX_RESIZE = 4500


def maybe(fnc, do=not DRY_RUN):
    if do:
        fnc()


def main():
    cnfs = []

    # CC modes are the outside loop to minimize how frequently we change the CC
    # mode, since doing so requires restarting the cluster.
    for cc in python_config.CCS:

        # (1) Long days, static buffers. Show the cases where TCP ramp up is not
        #     a problem.
        if cc in ["reno", "cubic"]:
            # Old optical switches.
            cnfs += [{"type": "strobe", "buffer_size": 16,
                      "night_len_us": 1000. * python_config.TDF,
                      "day_len_us": 9000. * python_config.TDF,
                      "cc": cc}]
            # New optical switches, but using long days.
            cnfs += [{"type": "strobe", "buffer_size": 16,
                      "night_len_us": python_config.RECONFIG_DELAY_us * \
                          python_config.TDF,
                      "day_len_us": 9000. * python_config.TDF, "cc": cc}]

        # (2) Static buffers. Show that all the TCP variants perform poorly when
        #     nights/days are short.
        # (3) reTCP. Show how much improvement reTCP offers with static buffers
        #     (i.e., on its own).
        cnf = {"type": "strobe", "buffer_size": 16, "cc": cc}
        # For DCTCP, we will enable threshold-based ECN marking.
        dctcp = cc == "dctcp"
        if dctcp:
            cnf["ecn"] = python_config.DCTCP_THRESH
        cnfs += [cnf]

        # (4) Dynamic buffers. Show that dynamic buffers help TCP Reno when
        #     nights/days are short.
        # (5) reTCP. Show how much improvement reTCP offers with dynamic
        #     buffers.
        if cc in ["reno", "retcp"]:
            for i in xrange(MAX_RESIZE + 1):
                if i % 500 == 0:
                    cnf = {"type": "strobe", "buffer_size":  16,
                           "queue_resize": True, "in_advance": i, "cc": cc}
                    if dctcp:
                        cnf["ecn"] = python_config.DCTCP_THRESH
                    cnfs += [cnf]

    # For all configurations, enable the packet log.
    cnfs = [dict(cnf, packet_log=True) for cnf in cnfs]

    # Use the first experiment's CC mode, or "reno" if no CC mode is specified.
    # This avoid unnecessarily restarting the cluster.
    maybe(lambda: common.initializeExperiment(
        "flowgrindd", cnfs[0].get("cc", "reno")))

    # Total number of experiments.
    tot = len(cnfs)
    for cnt, cnf in enumerate(cnfs, start=1):
        maybe(lambda: click_common.setConfig(cnf))
        print("--- config:\n{}".format(cnf))
        print("--- experiment {} of {}".format(cnt, tot))
        maybe(lambda: common.flowgrind(
            settings={"flows": [{"src": "r1", "dst": "r2"}]}))

    maybe(lambda: common.finishExperiment())


if __name__ == "__main__":
    main()
