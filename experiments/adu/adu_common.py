import itertools
buffer_sizes = [16]
in_advances = [12000]
ccs = ['reno']
# types = ['normal', 'circuit', 'no_circuit', 'strobe', 'resize']
types = ['normal']
traffic_sources = ['ADU', 'QUEUE']
CONFIGS = [{'type': t, 'buffer_size': b, 'cc': cc, 'in_advance': ia,
            'packet_log': False, 'traffic_source': ts}
           for (t, b, cc, ia, ts) in itertools.product(types, buffer_sizes, ccs,
                                                       in_advances, traffic_sources)]

CONFIGS = [
    {'type': 'normal', 'traffic_source': 'QUEUE'},
    {'type': 'resize', 'traffic_source': 'QUEUE', 'in_advance': 20000},
    {'type': 'resize', 'traffic_source': 'QUEUE', 'in_advance': 20000, 'cc': 'ocs'},
    {'type': 'normal', 'traffic_source': 'ADU'},
    {'type': 'resize', 'traffic_source': 'ADU', 'in_advance': 20000},
    {'type': 'resize', 'traffic_source': 'ADU', 'in_advance': 20000, 'cc': 'ocs'},
    {'type': 'no_circuit', 'traffic_source': 'QUEUE', 'buffer_size': 128, 'packet_link_bandwidth': 0.5},
    {'type': 'no_circuit', 'traffic_source': 'QUEUE', 'buffer_size': 128, 'packet_link_bandwidth': 2.0},
    {'type': 'no_circuit', 'traffic_source': 'QUEUE', 'buffer_size': 128, 'packet_link_bandwidth': 4.0},
    {'type': 'no_circuit', 'traffic_source': 'QUEUE', 'buffer_size': 128, 'packet_link_bandwidth': 4.5},
]

for c in CONFIGS:
    c['packet_log'] = False
    if c['type'] == 'resize':
        c['type'] = 'normal'
        c['queue_resize'] = True
