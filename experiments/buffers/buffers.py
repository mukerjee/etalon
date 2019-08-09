#!/usr/bin/env python

import sys
sys.path.insert(0, '/etalon/experiments')

import buffer_common
import click_common
import common


def main():
    common.initializeExperiment('flowgrindd')

    cnfs = buffer_common.CONFIGS
    # For every configuration, add a copy that uses reTCP as the CC mode. Put
    # the new configurations at the end so that the CC mode needs to be changed
    # only once.
    cnfs += [dict(cnf, {'cc': "retcp"}) for cnf in cnfs]
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
