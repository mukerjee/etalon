#!/usr/bin/env PYTHONPATH=../etc/ python

import sys
import os

from python_config import *

if __name__ == '__main__':
   scp = 'scp -o StrictHostKeyChecking=no %s:/etalon/experiments/%s/%s-*.tar.gz ./'
   switch = handle_to_machine('switch')
   os.system(scp % (switch, 'buffers', sys.argv[1]))
   os.system(scp % (switch, 'adu', sys.argv[1]))
   os.system(scp % (switch, 'hdfs', sys.argv[1]))
