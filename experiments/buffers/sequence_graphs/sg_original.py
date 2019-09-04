#!/usr/bin/env python

from os import path
import shelve
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For parse_logs.
sys.path.insert(0, path.join(PROGDIR, '..', '..'))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, '..', '..', '..', 'etc'))

import sg


def main():
    exp = sys.argv[1]
    if not path.isdir(exp):
        print("The first argument must be a directory, but is: {}".format(exp))
        sys.exit(-1)

    db = shelve.open(path.join(exp, 'seq_shelve.db'))
    sg.plot_seq(sg.get_data(db, 'static'), 'static')

    resize_data = sg.get_data(db, 'resize')
    # Use the same circuit windows as the 'static' experiment.
    resize_data['lines'] = db['static']['lines']
    # Use the data for 0 us from the 'static' experiment.
    resize_data['keys'] = [0] + db['resize']['keys']
    resize_data['data'] = [db['static']['data'][2]] + db['resize']['data']
    sg.plot_seq(resize_data, 'resize')

    db.close()


if __name__ == '__main__':
    main()
