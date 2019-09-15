#!/usr/bin/env python

from os import path
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For click_common and common.
sys.path.insert(0, path.join(PROGDIR, ".."))

from click_common import setConfig
from common import initializeExperiment, finishExperiment, flowgrind


def main():
    initializeExperiment('flowgrindd')

    configs = [{'type': 'no_circuit'}, {'type': 'circuit'}]
    for c in configs:
        c['cc'] = 'cubic'
        c['buffer_size'] = 128
        c['divert_acks'] = True
    for config in configs:
        print '--- running test type %s...' % config['type']
        print '--- setting switch buffer size to %d...' % config['buffer_size']
        setConfig(config)
        print '--- done...'

        settings = {'flows': []}
        # A ring: 1->2, 2->3, 3->1. Mimics DEFAULT_CIRCUIT_CONFIG.
        settings['flows'].append({'src': 'r1', 'dst': 'r2'})
        settings['flows'].append({'src': 'r2', 'dst': 'r3'})
        settings['flows'].append({'src': 'r3', 'dst': 'r1'})
        flowgrind(settings)

    finishExperiment()


if __name__ == "__main__":
    main()
