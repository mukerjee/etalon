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
    for cc in ["cubic"]:
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
        # Enable the hybrid switch's packet log. This should already be enabled
        # by default.
        cnf["packet_log"] = True
        if cnf["cc"] == "dctcp":
            # If the configuration uses DCTCP, then enable threshold-based ECN
            # marking.
            cnf["ecn"] = python_config.DCTCP_THRESH
        # If the night and day lengths have not been set already, then do so
        # here. Explicitly set the night and day lengths instead of relying on
        # their defaults so that we can automatically calculate the experiment
        # duration, below.
        if "night_len_us" not in cnf:
            cnf["night_len_us"] = 20 * python_config.TDF
            cnf["day_len_us"] = 180 * python_config.TDF

    # Run experiments. Use the first experiment's CC mode to avoid unnecessarily
    # restarting the cluster.
    maybe(lambda: common.initializeExperiment("flowgrindd", cnfs[0]["cc"]))
    # Total number of experiments.
    tot = len(cnfs)
    for cnt, cnf in enumerate(cnfs, start=1):
        maybe(lambda: click_common.setConfig(cnf))
        print("--- experiment {} of {}, config:\n{}".format(cnt, tot, cnf))
        maybe(lambda: common.flowgrind(settings={
            # Generate a flow from each machine on rack 1 to its corresponding
            # partner machine on rack 2.
            "flows": [{"src": "r1", "dst": "r2"}],
            # Run the flow for three thousand weeks plus 100 ms for good
            # measure, converted to seconds.
            "dur": (((cnf["night_len_us"] + cnf["day_len_us"])  # One night and day under TDF.
                     / python_config.TDF  # Convert from TDF to real time.
                     * (NUM_RACKS - 1 )  # Convert to a full week.
                     / 1e3  # Convert from us to ms.
                     * 3000  # 3000 weeks.
                     + 100)  # Extra 100 ms, for good measure.
                    / 1e3)  # Convert to seconds.
        }))
    maybe(common.finishExperiment)


if __name__ == "__main__":
    main()
