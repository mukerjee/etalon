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
    {'packet_log': False, 'type': 'normal', 'traffic_source': 'QUEUE'},
    {'packet_log': False, 'type': 'resize', 'traffic_source': 'QUEUE', 'in_advance': 16000},
    {'packet_log': False, 'type': 'resize', 'traffic_source': 'QUEUE', 'in_advance': 16000, 'cc': 'ocs'},
    {'packet_log': False, 'type': 'normal', 'traffic_source': 'ADU'},
    {'packet_log': False, 'type': 'resize', 'traffic_source': 'ADU', 'in_advance': 16000},
    {'packet_log': False, 'type': 'resize', 'traffic_source': 'ADU', 'in_advance': 16000, 'cc': 'ocs'},
]

for c in CONFIGS:
    if c['type'] == 'resize':
        c['type'] = 'normal'
        c['queue_resize'] = True
