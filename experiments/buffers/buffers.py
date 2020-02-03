#!/usr/bin/env python

import sys
sys.path.insert(0, '/etalon/experiments')

import buffer_common
import click_common
import common


def main():
    cnfs = buffer_common.gen_static_sweep(2, 7) + buffer_common.gen_resize_sweep(0, 8000, 500)
    # Use the first experiment's CC mode, or "reno" if no CC mode is specified.
    # This avoid unnecessarily restarting the cluster.
    common.initializeExperiment('flowgrindd', cnfs[0].get("cc", "reno"))

    # For every configuration, add a copy that uses reTCP as the CC mode. Put
    # the new configurations at the end so that the CC mode needs to be changed
    # only once.
    cnfs += [dict(cnf, {'cc': "retcp"}) for cnf in cnfs]
    tot = len(cnfs)
    for cnt, cnf in enumerate(cnfs, 1):
        print('--- running test type {}...'.format(cnf['type']))
        print('--- setting switch buffer size to {}...'.format(
            cnf['buffer_size']))
        click_common.setConfig(cnf)
        print('--- done...')
        print("--- experiment {} of {}".format(cnt, tot))
        common.flowgrind(settings={"flows": [{"src": "r1", "dst": "r2"}]})

    common.finishExperiment()


if __name__ == "__main__":
    main()
