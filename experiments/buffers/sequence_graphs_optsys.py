#!/usr/bin/env python
#
# This program generates graphs for experiments produced by
# etalon/experiments/buffers/optsys.py.

import copy
import os
from os import path
import shelve
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "..", "etc"))
# For parse_logs.
sys.path.insert(0, path.join(PROGDIR, ".."))

import parse_logs as par
import python_config as pyc
import sequence_graphs as sqg

# Durations.
OLD_DUR = (1000 + 9000) * 6
NEW_LONG_DUR = (20 + 9000) * 6
STATIC_DUR = (20 + 180) * 6
RESIZE_DUR = (20 + 180) * 6

# Inset window bounds.
OLD_INS = None
NEW_LONG_INS = None
STATIC_INS = None
# RESIZE_INS = ((620, 800), (50, 275))
RESIZE_INS = None

LINES_KEY = "lines"
DB_FMT = "{}.db"
OLD_KEY_FMT = "old-{}"
NEW_LONG_KEY_FMT = "new-long-{}"
STATIC_KEY_FMT = "static-{}"
RESIZE_KEY_FMT = "resize-{}"

# Matches experiments with a particular CC mode, 1000 us nights, and 9000 us
# days (under TDF).
OLD_FMT = "*-{}-*-20000-180000-click.txt"
# Matches experiments with a particular CC mode, 20 us nights, and 9000 us days
# (under TDF).
NEW_LONG_FMT = "*-{}-*-400-180000-click.txt"
# Matches experiments with static buffers of a particular size, a particular CC
# mode, 20 us nights, and 180 us days (under TDF).
STATIC_PTN_FMT = "*-{}-QUEUE-False-*-{}-*-400-3600-click.txt"
# Matches experiments with dynamic buffers, a particular resize time, and a
# particular CC mode.
RESIZE_PTN_FMT = "*-QUEUE-True-{}-{}-*click.txt"

# Primary CC modes to demonstrate.
BASIC_CCS = ["cubic", "reno"]
# CC mode indices to display in static graph.
DESIRED_CCS = ["optimal", "packet only", "bbr", "cubic", "dctcp", "highspeed",
               "illinois", "scalable", "westwood", "yeah"]
# DESIRED_CCS = [idx for idx in xrange(10)]
# DESIRED_CCS = [0] + [idx for idx in xrange(10, 19)]
# Resize time indices to display in resize graph.
DESIRED_RESIZE_US = [0, 1, 3, 5, 7, 8, 9, 11, 12]
# Resize time to graph for reTCP.
CHOSEN_RESIZE_US = int(175 * pyc.TDF)
# Resize CC mode.
RESIZE_CC = "cubic"
# Static buffer size to use.
CHOSEN_STATIC = 16


def rst_glb(dur):
    """ Reset global variables. """
    # Reset global lookup tables.
    sqg.FILES = {}
    sqg.KEY_FN = {}
    # Reset experiment duration.
    par.DURATION = dur
    # Do not set sqg.DURATION because it get configured automatically based on
    # the actual circuit timings.


def main():
    exp = sys.argv[1]
    if not path.isdir(exp):
        print("The first argument must be a directory, but is: {}".format(exp))
        sys.exit(-1)
    # Specify and create the output directory.
    odr = path.join(PROGDIR, 'graphs', 'optsys')
    if path.exists(odr):
        if not path.isdir(odr):
            print("Output directory exists and is a file: {}".format(odr))
            sys.exit(-1)
    else:
        os.makedirs(odr)

    # The circuit day boundaries.
    days = None
    for cc in BASIC_CCS:
        # (1) Long days, static buffers. Show the cases where TCP ramp up is not
        #     a problem.
        #
        # Old optical switches.
        rst_glb(OLD_DUR)
        old_key = OLD_KEY_FMT.format(cc)
        sqg.FILES[old_key] = OLD_FMT.format(cc)
        # Pass "cc" as a default parameter to avoid the warning
        # "cell-var-from-loop".
        sqg.KEY_FN[old_key] = lambda fn, cc=cc: cc
        old_db = shelve.open(path.join(exp, DB_FMT.format(old_key)))
        old_db[old_key] = sqg.get_data(old_db, old_key)
        sqg.plot_seq(old_db[old_key], old_key, odr, OLD_INS)
        old_db.close()

        # New optical switches, but using long days.
        rst_glb(NEW_LONG_DUR)
        new_long_key = NEW_LONG_KEY_FMT.format(cc)
        sqg.FILES[new_long_key] = NEW_LONG_FMT.format(cc)
        # Pass "cc" as a default parameter to avoid the warning
        # "cell-var-from-loop".
        sqg.KEY_FN[new_long_key] = lambda fn, cc=cc: cc
        new_long_db = shelve.open(path.join(exp, DB_FMT.format(new_long_key)))
        new_long_db[new_long_key] = sqg.get_data(new_long_db, new_long_key)
        sqg.plot_seq(new_long_db[new_long_key], new_long_key, odr, NEW_LONG_INS)
        new_long_db.close()

        # (2) Static buffers. Show that all the TCP variants perform poorly when
        #     nights/days are short. Start by graphing the basic CC modes by
        #     themselves, then graph all of the CC modes together.
        rst_glb(STATIC_DUR)
        static_key = STATIC_KEY_FMT.format(cc)
        # Match only the current CC mode.
        sqg.FILES[static_key] = STATIC_PTN_FMT.format(CHOSEN_STATIC, cc)
        # Extract the CC mode.
        sqg.KEY_FN[static_key] = lambda fn: fn.split("-")[7]
        static_db = shelve.open(path.join(exp, DB_FMT.format(static_key)))
        static_db[static_key] = sqg.get_data(static_db, static_key)
        # Use the same circuit windows for all graphs with short nights and
        # short days (i.e., (2) - (5)).
        if days is None:
            days = copy.deepcopy(static_db[static_key][LINES_KEY])
        else:
            static_db[static_key][LINES_KEY] = days
        sqg.plot_seq(static_db[static_key], static_key, odr, STATIC_INS)
        static_db.close()

        # (3) reTCP. Show how much improvement reTCP offers with static buffers
        #     (i.e., on its own).
        rst_glb(STATIC_DUR)
        static_key = STATIC_KEY_FMT.format("{}-retcp".format(cc))
        # Match the current CC mode and reTCP.
        sqg.FILES[static_key] = [STATIC_PTN_FMT.format(CHOSEN_STATIC, cc),
                                 STATIC_PTN_FMT.format(CHOSEN_STATIC, "retcp")]
        # Extract the CC mode.
        sqg.KEY_FN[static_key] = lambda fn: fn.split("-")[7]
        static_db = shelve.open(path.join(exp, DB_FMT.format(static_key)))
        static_db[static_key] = sqg.get_data(static_db, static_key)
        # Use the same circuit windows for all graphs with short nights and
        # short days (i.e., (2) - (5)).
        assert days is not None, "\"days\" is None!"
        sqg.plot_seq(static_db[static_key], static_key, odr, STATIC_INS)
        static_db.close()

        # (5) reTCP. Show how much improvement reTCP offers with dynamic
        #     buffers.
        rst_glb(RESIZE_DUR)
        resize_key = RESIZE_KEY_FMT.format(
            "{}-retcp-{}".format(cc, CHOSEN_RESIZE_US))
        # Match the resize time  us and both the current CC mode and retcp.
        sqg.FILES[resize_key] = [RESIZE_PTN_FMT.format(CHOSEN_RESIZE_US, cc),
                                 RESIZE_PTN_FMT.format(
                                     CHOSEN_RESIZE_US, "retcp")]
        # Extract the CC mode.
        sqg.KEY_FN[resize_key] = lambda fn: fn.split("-")[7]
        resize_db = shelve.open(path.join(exp, DB_FMT.format(resize_key)))
        resize_db[resize_key] = sqg.get_data(resize_db, resize_key)
        # Use the same circuit windows for all graphs with short nights and
        # short days (i.e., (2) - (5)).
        assert days is not None, "\"days\" is None!"
        resize_db[resize_key][LINES_KEY] = days
        sqg.plot_seq(resize_db[resize_key], resize_key, odr, RESIZE_INS)
        resize_db.close()

    # (2) Static buffers. See above. Now, graph all CC modes together.
    rst_glb(STATIC_DUR)
    static_key = STATIC_KEY_FMT.format("all")
    # Match any CC mode.
    sqg.FILES[static_key] = STATIC_PTN_FMT.format(CHOSEN_STATIC, "*")
    # Extract the CC mode.
    sqg.KEY_FN[static_key] = lambda fn: fn.split("-")[7]
    static_db = shelve.open(path.join(exp, DB_FMT.format(static_key)))
    static_db[static_key] = sqg.get_data(static_db, static_key)
    # Use the same circuit windows for all graphs with short nights and
    # short days (i.e., (2) - (5)).
    assert days is not None, "\"days\" is None!"
    static_db[static_key][LINES_KEY] = days
    sqg.plot_seq(static_db[static_key], static_key, odr, STATIC_INS,
                 flt=lambda idx, label, ccs=DESIRED_CCS: label in ccs)
    static_db.close()

    # (4) Dynamic buffers. Show that dynamic buffers help TCP Reno when
    #     nights/days are short.
    rst_glb(RESIZE_DUR)
    resize_key = RESIZE_KEY_FMT.format(RESIZE_CC)
    # Match any resize time, but only CC mode reno.
    sqg.FILES[resize_key] = [STATIC_PTN_FMT.format(CHOSEN_STATIC, RESIZE_CC),
                             RESIZE_PTN_FMT.format("*", RESIZE_CC)]
    # Extract how long in advance the buffers resize.
    sqg.KEY_FN[resize_key] = \
        lambda fn: int(float(fn.split("-")[6]) / pyc.TDF) \
        if "QUEUE-True" in fn else "static"
    resize_db = shelve.open(path.join(exp, DB_FMT.format(resize_key)))
    resize_db[resize_key] = sqg.get_data(resize_db, resize_key)
    # Use the same circuit windows for all graphs with short nights and short
    # days (i.e., (2) - (5)).
    assert days is not None, "\"days\" is None!"
    resize_db[resize_key][LINES_KEY] = days
    sqg.plot_seq(resize_db[resize_key],
                 resize_key,
                 odr,
                 RESIZE_INS,
                 flt=lambda idx, label, resize_us=DESIRED_RESIZE_US: \
                     idx in resize_us,
                 order=["optimal", "packet only", "static", "25", "75", "125", "150", "175", "225"])
    resize_db.close()


if __name__ == "__main__":
    main()
