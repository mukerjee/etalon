#!/usr/bin/env python
#
# This program parses experiments produced by
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
RESIZE_INS = None

LINES_KEY = "lines"
DB_FMT = "{}.db"
OLD_KEY_FMT = "old-{}"
NEW_LONG_KEY_FMT = "new-long-{}"
STATIC_KEY_FMT = "static-{}"
RESIZE_KEY = "resize"

# Matches experiments with a particular CC mode, 1000 us nights, and 9000 us
# days (under TDF).
OLD_FMT = "*-{}-*-20000-180000-click.txt"
# Matches experiments with a particular CC mode, 20 us nights, and 9000 us days
# (under TDF).
NEW_LONG_FMT = "*-{}-*-400-180000-click.txt"
# Matches experiments with static buffers, a particular CC mode, 20 us nights,
# and 180 us days (under TDF).
STATIC_PTN = "*-QUEUE-False-*-{}-*-400-3600-click.txt"
# Matches experiments with dynamic buffers and CC mode "reno".
RESIZE_PTN = "*-QUEUE-True-*-reno-*click.txt"

# Primary CC modes to demonstrate.
BASIC_CCS = ["cubic", "reno"]
# CC modes to display in bulk graphs.
DESIRED_CCS = [idx for idx in xrange(10)]


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
        # Match any CC mode.
        sqg.FILES[static_key] = STATIC_PTN.format(cc)
        # Extract the CC mode.
        sqg.KEY_FN[static_key] = lambda fn: fn.split("-")[7]
        static_db = shelve.open(path.join(exp, DB_FMT.format(static_key)))
        static_db[static_key] = sqg.get_data(static_db, static_key)
        # Use the same circuit windows for all graphs with short nights and
        # short days (i.e., (2) and (3)).
        if days is None:
            days = copy.deepcopy(static_db[static_key][LINES_KEY])
        else:
            static_db[static_key][LINES_KEY] = days
        sqg.plot_seq(static_db[static_key], static_key, odr, STATIC_INS)
        static_db.close()

    # (2) Static buffers. See above.
    rst_glb(STATIC_DUR)
    static_key = STATIC_KEY_FMT.format("all")
    # Match any CC mode.
    sqg.FILES[static_key] = STATIC_PTN.format("*")
    # Extract the CC mode.
    sqg.KEY_FN[static_key] = lambda fn: fn.split("-")[7]
    static_db = shelve.open(path.join(exp, DB_FMT.format(static_key)))
    static_db[static_key] = sqg.get_data(static_db, static_key)
    # Use the same circuit windows for all graphs with short nights and
    # short days (i.e., (2) and (3)).
    static_db[static_key][LINES_KEY] = days
    sqg.plot_seq(static_db[static_key], static_key, odr, STATIC_INS,
                 flt=lambda idx, ccs=DESIRED_CCS: idx in ccs)
    static_db.close()

    sys.exit(0)

    # (3) Dynamic buffers. Show that dynamic buffers help all TCP variants
    #     when nights/days are short. For now, only show this for reno.
    rst_glb(RESIZE_DUR)
    sqg.FILES[RESIZE_KEY] = RESIZE_PTN
    # Extract how long in advance the buffers resize.
    sqg.KEY_FN[RESIZE_KEY] = lambda fn: int(fn.split("-")[6]) / pyc.TDF,
    resize_db = shelve.open(path.join(exp, DB_FMT.format(RESIZE_KEY)))
    resize_db[RESIZE_KEY] = sqg.get_data(resize_db, RESIZE_KEY)
    # Use the same circuit windows for all graphs with short nights and short
    # days (i.e., (2) and (3)).
    resize_db[RESIZE_KEY][LINES_KEY] = days
    sqg.plot_seq(resize_db[RESIZE_KEY], RESIZE_KEY, odr, RESIZE_INS)
    resize_db.close()


if __name__ == "__main__":
    main()
