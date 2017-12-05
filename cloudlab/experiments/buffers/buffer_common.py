import itertools
# buffer_sizes = [4, 8, 16, 32, 64, 128]
buffer_sizes = [16]
days_outs = [3]
ccs = ['ocs']
# types = ['normal', 'no_circuit', 'strobe', 'resize']
types = ['resize']
CONFIGS = [{'type': t, 'buffer_size': b, 'cc': cc, 'days_out': do}
           for (t, b, cc, do) in itertools.product(types, buffer_sizes, ccs, days_outs)]

CONFIGS = [{'type': 'strobe', 'buffer_size': 4},
           {'type': 'strobe', 'buffer_size': 8},
           {'type': 'strobe', 'buffer_size': 16},
           {'type': 'strobe', 'buffer_size': 32},
           {'type': 'strobe', 'buffer_size': 64},
           {'type': 'strobe', 'buffer_size': 128},
           {'type': 'resize', 'buffer_size': 16, 'days_out': 1},
           {'type': 'resize', 'buffer_size': 16, 'days_out': 2},
           {'type': 'resize', 'buffer_size': 16, 'days_out': 3},
           {'type': 'resize', 'buffer_size': 16, 'days_out': 4},
           {'type': 'resize', 'buffer_size': 16, 'days_out': 5},
           {'type': 'strobe', 'buffer_size': 16, 'cc': 'ocs'},
           {'type': 'resize', 'buffer_size': 16, 'days_out': 1, 'cc': 'ocs'},
           {'type': 'resize', 'buffer_size': 16, 'days_out': 2, 'cc': 'ocs'},
           {'type': 'resize', 'buffer_size': 16, 'days_out': 3, 'cc': 'ocs'},
           {'type': 'resize', 'buffer_size': 16, 'days_out': 4, 'cc': 'ocs'},
           {'type': 'resize', 'buffer_size': 16, 'days_out': 5, 'cc': 'ocs'},
]

for c in CONFIGS:
    if c['type'] == 'resize':
        c['type'] = 'strobe'
        c['queue_resize'] = True
