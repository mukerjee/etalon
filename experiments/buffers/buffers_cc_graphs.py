#!/usr/bin/env python

import os
import sys

import buffers_cc
import buffers_graphs

FILES_BASE = {
    'static': '/*-strobe-*-False-*-reno-*click.txt',
    'resize': '/*-QUEUE-True-*-reno-*click.txt',
}
FILES = {"{}_{}".format(typ, cc) : bas.format(cc)
         for typ, bas in FILES_BASE.items()
         for cc in buffers_cc.CCS}

FILES = {}
KEY_FN = {}

if __name__ == "__main__":
    assert len(sys.argv) == 2, "Requires exactly one argument."
    if not os.path.isdir(sys.argv[1]):
        print 'first arg must be dir'
        sys.exit(-1)
    db = shelve.open(sys.argv[1] + '/buffer_shelve.db')
    print FILES
    
