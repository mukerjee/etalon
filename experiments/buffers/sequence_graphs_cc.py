#!/usr/bin/env python

import copy
import os
from os import path
import shelve
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "..", "etc"))

import python_config
import sequence_graphs


def main():
    exp = sys.argv[1]
    if not path.isdir(exp):
        print("The first argument must be a directory, but is: {}".format(exp))
        sys.exit(-1)

    # Create entries for each CC mode. Keys are of the form "resize-<CC mode>".
    fmt = "resize-{}"
    for cc in python_config.CCS:
        sequence_graphs.FILES[fmt.format(cc)] = \
            "/*-QUEUE-True-*-{}-*click.txt".format(cc)
        sequence_graphs.KEY_FN[fmt.format(cc)] = \
            sequence_graphs.KEY_FN["resize"]

    # Parse the static data to figure out the 0 us case. Copy the results and
    # store the database file.
    db = shelve.open(path.join(exp, "seq_static_shelve.db"))
    db["static"] = sequence_graphs.get_data(db, "static")
    dbs = copy.deepcopy(db["static"])
    db.close()

    # Create a graph for each CC mode.
    for cc in python_config.CCS:
        # Use a new database for each CC mode to avoid storing everything in
        # memory at once. This also enables the program to be killed and
        # restarted partway through without losing progress.
        db = shelve.open(path.join(exp, "seq_{}_shelve.db".format(cc)))
        key = fmt.format(cc)
        db[key] = sequence_graphs.get_data(db, key)
        cc_data = copy.deepcopy(db[key])
        db.close()
        # Use the same circuit windows for all graphs.
        cc_data["lines"] = dbs["lines"]
        # Use the data for 0 us from the "static" experiment.
        cc_data["keys"] = [0] + cc_data["keys"]
        cc_data["data"] = [dbs["data"][2]] + cc_data["data"]
        sequence_graphs.plot_seq(cc_data, key)


if __name__ == "__main__":
    main()
