#!/usr/bin/env python

from os import path
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For click_common and common.
sys.path.insert(0, path.join(PROGDIR, ".."))

import click_common
import common

CC = "cubic"
BUFFER_SIZE = 128


def main():
    common.initializeExperiment("flowgrindd", cc=CC)

    for typ in ["no_circuit", "circuit", "strobe"]:
        cnf = {"type": typ,
               "cc": CC,
               "buffer_size": BUFFER_SIZE,
               "divert_acks": True}
        click_common.setConfig(cnf)
        print("--- config:\n{}".format(cnf))
        common.flowgrind(settings={"flows": [{"src": "r1", "dst": "r2"}]})

    common.finishExperiment()


if __name__ == "__main__":
    main()
