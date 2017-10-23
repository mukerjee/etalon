import itertools
# buffer_sizes = [1, 5, 10, 20, 30, 40, 50, 75, 100]
# buffer_sizes = [10, 20, 30, 40, 50, 75, 100, 200]
buffer_sizes = [100]
# types = ['normal', 'resize'] #, 'no_circuit', 'strobe']
types = ['strobe', 'resize']
CONFIGS = [{'type': t, 'buffer_size': b}
           for (t, b) in itertools.product(types, buffer_sizes)]

for c in CONFIGS:
    if c['type'] == 'resize':
        c['type'] = 'strobe'
        c['queue_resize'] = True
