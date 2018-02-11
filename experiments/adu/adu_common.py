CONFIGS = [
    {'type': 'normal', 'traffic_source': 'QUEUE'},
    {'type': 'resize', 'traffic_source': 'QUEUE', 'in_advance': 20000},
    {'type': 'resize', 'traffic_source': 'QUEUE', 'in_advance': 20000, 'cc': 'retcp'},
    {'type': 'normal', 'traffic_source': 'ADU'},
    {'type': 'resize', 'traffic_source': 'ADU', 'in_advance': 20000},
    {'type': 'resize', 'traffic_source': 'ADU', 'in_advance': 20000, 'cc': 'retcp'},
    {'type': 'no_circuit', 'traffic_source': 'QUEUE', 'buffer_size': 128, 'packet_link_bandwidth': 0.5},
    {'type': 'no_circuit', 'traffic_source': 'QUEUE', 'buffer_size': 128, 'packet_link_bandwidth': 1.0},
    {'type': 'no_circuit', 'traffic_source': 'QUEUE', 'buffer_size': 128, 'packet_link_bandwidth': 2.0},
    {'type': 'no_circuit', 'traffic_source': 'QUEUE', 'buffer_size': 128, 'packet_link_bandwidth': 4.0},
    {'type': 'no_circuit', 'traffic_source': 'QUEUE', 'buffer_size': 128, 'packet_link_bandwidth': 4.5},
]

for c in CONFIGS:
    c['packet_log'] = False
    if c['type'] == 'resize':
        c['type'] = 'normal'
        c['queue_resize'] = True
