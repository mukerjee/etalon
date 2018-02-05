NUM_RACKS = 8
HOSTS_PER_RACK = 16

NIC = 'enp8s0'
DATA_NET = 1

TDF = 20.0

CIRCUIT_BW = 80 / TDF
PACKET_BW = 10 / TDF

PACKET_LATENCY = 0.000100
CIRCUIT_LATENCY = 0.000600

RECONFIG_DELAY = 20

CONTROL_SOCKET_PORT = 1239

NODES_FILE = '/etalon/etc/handles.cloudlab'

def handle_to_machine(h):
    machines = open(NODES_FILE, 'r').read().split('\n')[:-1]
    for machine in machines:
        handle, hostname = [m.strip() for m in machine.split('#')]
        if handle == h:
            return handle
    return None

