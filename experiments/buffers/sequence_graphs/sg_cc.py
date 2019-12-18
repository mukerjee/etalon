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
    assert len(sys.argv) == 3, \
        "Expected one argument: experiment data dir, output dir"
    edr, odr = sys.argv[1:]
    if not path.isdir(edr):
        print("The first argument must be a directory, but is: {}".format(edr))
        sys.exit(-1)
    if path.exists(odr):
        if not path.isdir(odr):
            print("Output directory exists and is a file: {}".format(odr))
            sys.exit(-1)
    else:
        os.makedirs(odr)

    # Create a graph for each TCP variant.
    for var in ["cubic"]:
        sg.seq(
            name="seq-dyn-{}".format(var),
            edr=edr,
            odr=odr,
            ptn="*-QUEUE-True-*-{}-*click.txt".format(var),
            key_fnc=lambda fn: int(round(float(fn.split("-")[6])
                                         / python_config.TDF)),
            dur=1200,
            # chunk_mode=100,
            msg_len=116,
            log_pos="after")
            # ins=((2780, 2960), (200, 340)),
            #flt=lambda idx, label: idx < 10)
            # flt=lambda idx, label: idx in [0, 1, 3, 4, 5, 6, 7, 8, 9, 10])


if __name__ == "__main__":
    main()
