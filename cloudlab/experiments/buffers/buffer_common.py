import itertools
# buffer_sizes = [4, 8, 16, 32, 64, 128]
buffer_sizes = [16]
days_outs = [3]
ccs = ['reno']
# types = ['normal', 'no_circuit', 'strobe', 'resize']
types = ['resize']
CONFIGS = [{'type': t, 'buffer_size': b, 'cc': cc, 'days_out': do}
           for (t, b, cc, do) in itertools.product(types, buffer_sizes, ccs, days_outs)]

for c in CONFIGS:
    if c['type'] == 'resize':
        c['type'] = 'strobe'
        c['queue_resize'] = True
