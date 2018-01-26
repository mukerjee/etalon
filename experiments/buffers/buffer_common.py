import itertools
buffer_sizes = [16]
in_advances = [12000]
ccs = ['reno']
# types = ['normal', 'circuit', 'no_circuit', 'strobe', 'resize']
types = ['strobe']
CONFIGS = [{'type': t, 'buffer_size': b, 'cc': cc, 'in_advance': ia,
            'packet_log': False}
           for (t, b, cc, ia) in itertools.product(types, buffer_sizes, ccs,
                                                   in_advances)]

CONFIGS = [
    {'type': 'strobe', 'buffer_size':   4},
    {'type': 'strobe', 'buffer_size':   8},
    {'type': 'strobe', 'buffer_size':  16},
    {'type': 'strobe', 'buffer_size':  32},
    {'type': 'strobe', 'buffer_size':  64},
    {'type': 'strobe', 'buffer_size': 128},
    {'type': 'circuit', 'buffer_size':   4},
    {'type': 'circuit', 'buffer_size':   8},
    {'type': 'circuit', 'buffer_size':  16},
    {'type': 'circuit', 'buffer_size':  32},
    {'type': 'circuit', 'buffer_size':  64},
    {'type': 'circuit', 'buffer_size': 128},
    {'type': 'no_circuit', 'buffer_size':   4},
    {'type': 'no_circuit', 'buffer_size':   8},
    {'type': 'no_circuit', 'buffer_size':  16},
    {'type': 'no_circuit', 'buffer_size':  32},
    {'type': 'no_circuit', 'buffer_size':  64},
    {'type': 'no_circuit', 'buffer_size': 128},
    {'type': 'resize', 'buffer_size':  16, 'in_advance':  4000},
    {'type': 'resize', 'buffer_size':  16, 'in_advance':  8000},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 12000},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 16000},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 20000},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 24000},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 28000},
    {'type': 'strobe', 'buffer_size':  16, 'cc': 'ocs'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance':  4000, 'cc': 'ocs'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance':  8000, 'cc': 'ocs'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 12000, 'cc': 'ocs'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 16000, 'cc': 'ocs'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 20000, 'cc': 'ocs'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 24000, 'cc': 'ocs'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 28000, 'cc': 'ocs'},
]

for c in CONFIGS:
    if c['type'] == 'resize':
        c['type'] = 'strobe'
        c['queue_resize'] = True
