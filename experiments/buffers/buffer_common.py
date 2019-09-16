

def gen_static_sweep(mn, mx):
    return [{
        # Enable the hybrid switch's packet log. This should already be enabled
        # by default.
        'packet_log': True,
        'type': 'strobe',
        'buffer_size': 2**exp
        } for exp in xrange(mn, mx + 1)]


def gen_resize_sweep(mn, mx, dl):
    return [{
        # Enable the hybrid switch's packet log. This should already be enabled
        # by default.
        'packet_log': True,
        'type': 'strobe',
        'queue_resize': True,
        'buffer_size': 16,
        'in_advance': us
    } for us in xrange(mn, mx + 1, dl)]

CONFIGS = gen_static_sweep(2, 7) + gen_resize_sweep(0, 8000, 500)
