#!/usr/bin/env python

import sys
sys.path.insert(0, '/etalon/experiments')

import buffer_common
import click_common
import common


def main():
    common.initializeExperiment('flowgrindd')

    # Total number of experiments.
    cnfs = buffer_common.CONFIGS
    tot = len(cnfs)
    for cnt, cnf in enumerate(cnfs):
        print('--- running test type {}...'.format(cnf['type']))
        print('--- setting switch buffer size to {}...'.format(
            cnf['buffer_size']))
        click_common.setConfig(cnf)
        print('--- done...')
        print("--- experiment {} of {}".format(cnt + 1, tot))
        common.flowgrind(settings={"flows": [{"src": "r1", "dst": "r2"}]})

    common.finishExperiment()


if __name__ == "__main__":
    main()
