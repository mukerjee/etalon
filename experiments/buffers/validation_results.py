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
    edr = sys.argv[1]
    if not path.isdir(edr):
        print("The first argument must be a directory, but is: {}".format(edr))
        sys.exit(-1)

    parse_logs.parse_validation_log(
        glob.glob(path.join(edr, '*-validation-no_circuit-*-flowgrind.txt'))[0])
    parse_logs.parse_validation_log(
        glob.glob(path.join(edr, '*-validation-circuit-*-flowgrind.txt'))[0])


if __name__ == '__main__':
    main()
