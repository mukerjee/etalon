import time
import sys
import os

NUM_RACKS = 2
HOSTS_PER_RACK = 8
TIMESTAMP = int(time.time())
SCRIPT = os.path.splitext(os.path.basename(sys.argv[0]))[0]
