

def gen_static_sweep(mn, mx):
    # Generate a sweep of configuration with different static buffer sizes.
    #
    # mn: minimum buffer size
    # mx: maximum buffer size
    #
    # mn and mx are in terms of powers of 2 (e.g., mn = 7 -> 2^7 = 128 packets).
    return [{
        # Enable the hybrid switch's packet log. This should already be enabled
        # by default.
        'packet_log': True,
        'type': 'strobe',
        'buffer_size': 2**exp
        } for exp in xrange(mn, mx + 1)]


def gen_resize_sweep(mn, mx, dl):
    # Generate a sweep of configurations with different amounts of dynamic
    # buffer resizing.
    #
    # mn: minimum resizing duration
    # mx: maximum resizing duration
    # dl: delta by which to increase the resizing duration.
    #
    # mn, mx, and dl are in terms of microseconds.
    return [{
        # Enable the hybrid switch's packet log. This should already be enabled
        # by default.
        'packet_log': True,
        'type': 'strobe',
        'queue_resize': True,
        'buffer_size': 16,
        'in_advance': us
    } for us in xrange(mn, mx + 1, dl)]
