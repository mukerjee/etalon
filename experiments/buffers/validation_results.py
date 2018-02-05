#!/usr/bin/env PYTHONPATH=../ python

import sys
import os
import glob

from parse_logs import parse_validation_log

    
if __name__ == '__main__':
    if not os.path.isdir(sys.argv[1]):
        print 'first arg must be dir'
        sys.exit(-1)
    parse_validation_log(sys.argv[1], glob.glob(sys.argv[1] +
                                                '/*-verification-no_circuit-*-'
                                                'flowgrind.txt'), True)
    
    parse_validation_log(sys.argv[1], glob.glob(sys.argv[1] +
                                                '/*-verification-circuit-*-'
                                                'flowgrind.txt'), False)
