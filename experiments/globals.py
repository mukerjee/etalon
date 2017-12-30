import time
import sys
import os

NUM_RACKS = 8
HOSTS_PER_RACK = 16
TDF = 20
TIMESTAMP = int(time.time())
SCRIPT = os.path.splitext(os.path.basename(sys.argv[0]))[0]
EXPERIMENTS = []
PHYSICAL_NODES = []

PACKET_BW = 10 * 10**9
CIRCUIT_BW = 80 * 10**9

RPYC_PORT = 18861
RPYC_CONNECTIONS = {}
