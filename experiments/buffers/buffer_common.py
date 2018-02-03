CONFIGS = [
    {'type': 'strobe', 'buffer_size':   4},
    {'type': 'strobe', 'buffer_size':   8},
    {'type': 'strobe', 'buffer_size':  16},
    {'type': 'strobe', 'buffer_size':  32},
    {'type': 'strobe', 'buffer_size':  64},
    {'type': 'strobe', 'buffer_size': 128},
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
