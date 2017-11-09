import time
import sys
import os

NUM_RACKS = 8
HOSTS_PER_RACK = 8
TDF = 20
TIMESTAMP = int(time.time())
SCRIPT = os.path.splitext(os.path.basename(sys.argv[0]))[0]
EXPERIMENTS = []
