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
DRY_RUN = True
# If True, then collect tcpdump traces for every experiment.
TCPDUMP = False
# If True, then racks will be launched in serial.
SYNC = False
# The number of racks to mimic when creating the strobe schedule.
NUM_RACKS_FAKE = 25
# Run static buffer experiments up to buffer size 2**STATIC_POW_MAX.
STATIC_POW_MAX = 7
# Long prebuffering sweep bounds.
RESIZE_LONG_MIN_us = 0
RESIZE_LONG_MAX_us = 3000
RESIZE_LONG_DELTA_us = 300
ALL_VARIANTS_uss = [1200]
# Short prebuffering sweep bounds.
RESIZE_SHORT_MIN_us = 0
RESIZE_SHORT_MAX_us = 300
RESIZE_SHORT_DELTA_us = 50
# VOQ capacities.
SMALL_QUEUE_CAP = 16
BIG_QUEUE_CAP = 50


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
            # (1) Old switches.
            cnfs += [{"type": "fake_strobe",
                      "num_racks_fake": NUM_RACKS_FAKE,
                      "night_len_us": 1000 * python_config.TDF,
                      "day_len_us": 9000 * python_config.TDF,
                      "cc": cc}]
            # (2) Future switches.
            cnfs += [{"type": "fake_strobe",
                      "num_racks_fake": NUM_RACKS_FAKE,
                      "night_len_us": 1 * python_config.TDF,
                      "day_len_us": 9 * python_config.TDF, "cc": cc}]
        # (3) Static buffers.
        for exp in xrange(2, STATIC_POW_MAX + 1):
            # Only do full sweeps for CUBIC and reTCP, but capture 16 packets
            # for all variants.
            if cc in ["cubic", "retcp"] or exp == 4:
                # Note that this configuration does not use buffer resizing, so
                # we do not *need* to set "big_queue_cap". We set
                # "big_queue_cap" anyway to improve error checking in the Click
                # element RunSchedule.
                cnfs += [{"type": "fake_strobe",
                          "num_racks_fake": NUM_RACKS_FAKE,
                          "small_queue_cap": 2**exp,
                          "big_queue_cap": 2**exp,
                          "cc": cc}]
        # (4) Long prebuffering.
        for us in xrange(RESIZE_LONG_MIN_us, RESIZE_LONG_MAX_us + 1,
                         RESIZE_LONG_DELTA_us):
            # Only do a full sweep for CUBIC, but capture a few key us's for all
            # variants.
            if cc == "cubic" or us in ALL_VARIANTS_uss:
                cnfs += [{"type": "fake_strobe",
                          "num_racks_fake": NUM_RACKS_FAKE,
                          "queue_resize": True,
                          "in_advance": int(round(us * python_config.TDF)),
                          "cc": cc}]
        # (4) Short prebuffering, only for reTCP.
        if cc == "retcp":
            for us in xrange(RESIZE_SHORT_MIN_us, RESIZE_SHORT_MAX_us + 1,
                             RESIZE_SHORT_DELTA_us):
                cnfs += [{"type": "fake_strobe",
                          "num_racks_fake": NUM_RACKS_FAKE,
                          "queue_resize": True,
                          "in_advance": int(round(us * python_config.TDF)),
                          "cc": cc}]
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
            cnf["night_len_us"] = int(round(20 * python_config.TDF))
            cnf["day_len_us"] = int(round(180 * python_config.TDF))
        if "small_queue_cap" not in cnf:
            cnf["small_queue_cap"] = SMALL_QUEUE_CAP
            cnf["big_queue_cap"] = BIG_QUEUE_CAP

    # Assemble settings. Generate the list of settings first so that we can
    # the estimated total duration.
    cnfs = [
        (cnf, {
            # Generate a flow from each machine on rack 1 to its corresponding
            # partner machine on rack 2.
            "flows": [{"src": "r2", "dst": "r3"}],
            # Run the flow for three thousand weeks plus 100 ms for good
            # measure, converted to seconds. The resulting duration is not under
            # TDF, but the flow will be under TDF when it is executed (i.e., the
            # flow will actually take TDF times longer than the value computed
            # here).
            "dur": (((cnf["night_len_us"] + cnf["day_len_us"])  # One night and day under TDF.
                     / python_config.TDF                        # Convert from TDF to real time.
                     * (cnf["num_racks_fake"] - 1)              # Convert to a full week.
                     / 1e3                                      # Convert from us to ms.
                     * 3000                                     # 3000 weeks.
                     + 100)                                     # Extra 100 ms, for good measure.
                    / 1e3),                                     # Convert to seconds.
            "tcpdump": TCPDUMP
        }) for cnf in cnfs]

    # Total number of experiments.
    tot = len(cnfs)
    # Estimated total duration.
    dur = sum([stg["dur"] for cnf, stg in cnfs]) * python_config.TDF
    print("Estimated total duration: >{} seconds".format(dur))
    # Run experiments. Use the first experiment's CC mode to avoid unnecessarily
    # restarting the cluster.
    maybe(lambda: common.initializeExperiment(
        "flowgrindd", cc=cnfs[0][0]["cc"], sync=SYNC))
    for cnt, (cnf, stg)  in enumerate(cnfs, start=1):
        maybe(lambda: click_common.setConfig(cnf))
        print("--- experiment {} of {}, config:\n{}".format(cnt, tot, cnf))
        maybe(lambda: common.flowgrind(settings=stg))
    maybe(common.finishExperiment)


if __name__ == "__main__":
    main()
