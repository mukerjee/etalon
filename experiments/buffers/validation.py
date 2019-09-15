#!/usr/bin/env python

from os import path
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For click_common and common.
sys.path.insert(0, path.join(PROGDIR, ".."))

import click_common
import common


def main():
    common.initializeExperiment("flowgrindd")

    for typ in ["no_circuit", "circuit", "strobe"]:
        cnf = {"type": typ,
               "cc": "cubic",
               "buffer_size": 128,
               "divert_acks": True}
        click_common.setConfig(cnf)
        print("--- config:\n{}".format(cnf))

        # A ring: 1->2, 2->3, 3->1. Mimics DEFAULT_CIRCUIT_CONFIG.
        click_common.flowgrind(settings={
            "flows": [
                {"src": "r1", "dst": "r2"},
                {"src": "r2", "dst": "r3"},
                {"src": "r3", "dst": "r1"}]})


    common.finishExperiment()


if __name__ == "__main__":
    main()
