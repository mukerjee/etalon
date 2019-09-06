#!/usr/bin/env python

from os import path
import shelve
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "..", "..", "etc"))

import python_config
import sg


def main():
    exp = sys.argv[1]
    if not path.isdir(exp):
        print("The first argument must be a directory, but is: {}".format(exp))
        sys.exit(-1)

    # Create entries for each CC mode. Keys are of the form "resize-<CC mode>".
    ccs = python_config.CCS
    fmt = "resize-{}"
    for cc in ccs:
        sg.FILES[fmt.format(cc)] = "*-QUEUE-True-*-{}-*click.txt".format(cc)
        sg.KEY_FN[fmt.format(cc)] = sg.KEY_FN["resize"]

    # Create a graph for each CC mode.
    lines = None
    ins = ((2600, 2820), (50, 450))
    flt = lambda idx, label: idx < 10  # in [0, 2, 3, 4, 5, 6, 7, 8, 9, 17]
    for cc in ccs:
        # Use a new database for each CC mode to avoid storing everything in
        # memory at once. This also enables the program to be killed and
        # restarted partway through without losing progress.
        db = shelve.open(path.join(exp, "seq_{}_shelve.db".format(cc)))
        key = fmt.format(cc)
        cc_data = sg.get_data(db, key)
        db.close()
        # Use the same circuit windows for all graphs.
        if lines is None:
            lines = cc_data["lines"]
        else:
            cc_data["lines"] = lines

        sg.plot_seq(cc_data, key, ins=ins, flt=flt)


if __name__ == "__main__":
    main()
