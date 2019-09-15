#!/usr/bin/env python

import glob
from os import path
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For parse_logs.
sys.path.insert(0, path.join(PROGDIR, ".."))

import parse_logs


def main():
    assert len(sys.argv) == 2, "Expected one argument: experiment dir"
    edr = sys.argv[1]
    if not path.isdir(edr):
        print("The first argument must be a directory, but is: {}".format(edr))
        sys.exit(-1)

    for ptn in ["*-no_circuit-*-flowgrind.txt",
                "*-circuit-*-flowgrind.txt",
                "*-strobe-*-flowgrind.txt"]:
        parse_logs.parse_validation_log(glob.glob(path.join(edr, ptn))[0])


if __name__ == "__main__":
    main()
