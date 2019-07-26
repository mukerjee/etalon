
CONFIGS = [
    {'type': 'strobe', 'buffer_size':   4},
    {'type': 'strobe', 'buffer_size':   8},
    {'type': 'strobe', 'buffer_size':  16},
    {'type': 'strobe', 'buffer_size':  32},
    {'type': 'strobe', 'buffer_size':  64},
    {'type': 'strobe', 'buffer_size': 128},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 500},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 1000},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 1500},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 2000},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 2500},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 3000},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 3500},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 4000},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 4500},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 5000},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 5500},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 6000},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 6500},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 7000},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 7500},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 8000},
    {'type': 'strobe', 'buffer_size':  16, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 500, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 1000, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 1500, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 2000, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 2500, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 3000, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 3500, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 4000, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 4500, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 5000, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 5500, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 6000, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 6500, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 7000, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 7500, 'cc': 'retcp'},
    {'type': 'resize', 'buffer_size':  16, 'in_advance': 8000, 'cc': 'retcp'},
]


for c in CONFIGS:
    # Enable Click's HSLog packet log. This should already be enabled by default.
    c['packet_log'] = True

    if c['type'] == 'resize':
        c['type'] = 'strobe'
        c['queue_resize'] = True
