import itertools
buffer_sizes = [16]
in_advances = [12000]
ccs = ['reno']
# types = ['normal', 'circuit', 'no_circuit', 'strobe', 'resize']
types = ['normal']
traffic_sources = ['QUEUE', 'ADU']
CONFIGS = [{'type': t, 'buffer_size': b, 'cc': cc, 'in_advance': ia,
            'packet_log': False, 'traffic_source': ts}
           for (t, b, cc, ia, ts) in itertools.product(types, buffer_sizes, ccs,
                                                       in_advances, traffic_sources)]

# CONFIGS = [
#     {'type': 'strobe', 'buffer_size':   4},
#     {'type': 'strobe', 'buffer_size':   8},
#     {'type': 'strobe', 'buffer_size':  16},
#     {'type': 'strobe', 'buffer_size':  32},
#     {'type': 'strobe', 'buffer_size':  64},
#     {'type': 'strobe', 'buffer_size': 128},
#     {'type': 'resize', 'buffer_size':  16, 'in_advance':  4000},
#     {'type': 'resize', 'buffer_size':  16, 'in_advance':  8000},
#     {'type': 'resize', 'buffer_size':  16, 'in_advance': 12000},
#     {'type': 'resize', 'buffer_size':  16, 'in_advance': 16000},
#     {'type': 'resize', 'buffer_size':  16, 'in_advance': 20000},
#     {'type': 'strobe', 'buffer_size':  16, 'cc': 'ocs'},
#     {'type': 'resize', 'buffer_size':  16, 'in_advance':  4000, 'cc': 'ocs'},
#     {'type': 'resize', 'buffer_size':  16, 'in_advance':  8000, 'cc': 'ocs'},
#     {'type': 'resize', 'buffer_size':  16, 'in_advance': 12000, 'cc': 'ocs'},
#     {'type': 'resize', 'buffer_size':  16, 'in_advance': 16000, 'cc': 'ocs'},
#     {'type': 'resize', 'buffer_size':  16, 'in_advance': 20000, 'cc': 'ocs'},
# ]

for c in CONFIGS:
    if c['type'] == 'resize':
        c['type'] = 'normal'
        c['queue_resize'] = True
