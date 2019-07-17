#!/usr/bin/env python

import copy
import os
from os import path
import shelve
import sys
# For python_config.
PROGDIR = path.dirname(path.realpath(__file__))
sys.path.insert(0, path.join(PROGDIR, "..", "..", "etc"))

import python_config
import sequence_graphs


def main():
    if not os.path.isdir(sys.argv[1]):
        print('first arg must be dir')
        sys.exit(-1)

    # Create entries for each CC mode. Keys are of the form "resize-<CC mode>".
    fmt = "resize-{}"
    for ccm in python_config.CCMS:
        sequence_graphs.files[fmt.format(ccm)] = \
            '/*-QUEUE-True-*-{}-*click.txt'.format(ccm)
    for ccm in python_config.CCMS:
        sequence_graphs.key_fn[fmt.format(ccm)] = \
            sequence_graphs.key_fn['resize']

    # Parse the static data to figure out the 0 us case. Copy the results and
    # store the database file.
    db = shelve.open(sys.argv[1] + '/seq_shelve.db')
    db['static'] = sequence_graphs.get_data(db, 'static')
    dbs = copy.deepcopy(db['static'])
    db.close()

    # Create a graph for each CC mode.
    for ccm in python_config.CCMS:
        # Use a new database for each CC mode to avoid storing everything in
        # memory as once.
        db = shelve.open(path.join(sys.argv[1], 'seq_{}_shelve.db'.format(ccm)))
        key = "resize-{}".format(ccm)
        db[key] = sequence_graphs.get_data(db, key)
        ccm_data = copy.deepcopy(db[key])
        # Use the same circuit windows for all graphs.
        ccm_data['lines'] = dbs['lines']
        # Use the data for 0 us from the 'static' experiment.
        ccm_data['keys'] = [0] + db[key]['keys']
        ccm_data['data'] = [dbs['data'][2]] + db[key]['data']
        sequence_graphs.plot_seq(ccm_data, key)
        db.close()


if __name__ == "__main__":
    main()
