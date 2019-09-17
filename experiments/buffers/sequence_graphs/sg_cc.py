#!/usr/bin/env python

import os
from os import path
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "..", "..", "etc"))

import python_config
import sg


def main():
    assert len(sys.argv) == 2, \
        "Expected one argument: experiment data directory"
    edr = sys.argv[1]
    if not path.isdir(edr):
        print("The first argument must be a directory, but is: {}".format(edr))
        sys.exit(-1)
    # Specify and create the output directory.
    odr = path.join(PROGDIR, "..", "graphs", "nsdi2020")
    if path.exists(odr):
        if not path.isdir(odr):
            print("Output directory exists and is a file: {}".format(odr))
            sys.exit(-1)
    else:
        os.makedirs(odr)

    # Create a graph for each CC mode.
    for cc in python_config.CCS:
        sg.seq(
            name="seq-dyn-{}".format(cc),
            edr=edr,
            odr=odr,
            ptn="*-QUEUE-True-*-{}-*click.txt".format(cc),
            key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                         / python_config.TDF)),
            dur=1200,
            flt=lambda idx, label: idx < 10)  # in [0, 2, 3, 4, 5, 6, 7, 8, 9, 17]


if __name__ == "__main__":
    main()
