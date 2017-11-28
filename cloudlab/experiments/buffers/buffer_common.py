import itertools
# buffer_sizes = [1, 5, 10, 20, 30, 40, 50, 75, 100]
# buffer_sizes = [10, 20, 30, 40, 50, 75, 100, 200]
# buffer_sizes = [8, 16, 32, 48, 64, 96, 128, 200, 256, 512, 1024]
# buffer_sizes = [64, 96, 128, 200, 256]
buffer_sizes = [16]
# mark_fractions = [0.55, 0.60, 0.65, 0.70, 0.75, 0.80, 0.85]
mark_fractions = [0.25]
# types = ['normal', 'no_circuit', 'strobe', 'resize']
types = ['strobe']
CONFIGS = [{'type': t, 'buffer_size': b, 'mark_fraction': mf}
           for (t, b, mf) in itertools.product(types, buffer_sizes, mark_fractions)]

for c in CONFIGS:
    if c['type'] == 'resize':
        c['type'] = 'strobe'
        c['queue_resize'] = True
