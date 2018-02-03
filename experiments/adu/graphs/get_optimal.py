#!/usr/bin/env python

import sys
import glob
import time
import gurobipy

from collections import defaultdict


def get_flow_starts_and_bytes(fn):
    flows = []
    for l in open(fn.split('.txt')[0] + '.config.txt'):
        l = l.split('-F')[1:]
        for x in l:
            src = int(x.strip().split('-Hs=10.1.')[1].split('.')[0]) - 1
            dst = int(x.strip().split(',d=10.1.')[1].split('.')[0]) - 1
            start = float(x.strip().split('-Ys=')[1].split()[0]) * 1e6
            bytes = int(x.strip().split('-Gs=q:C:')[1].split()[0])
            if src == dst:
                continue
            flows.append({'src': src, 'dst': dst,
                          'start': start, 'bytes': bytes})
            return flows
    return flows


def get_optimal_fct(fn):
    flows = get_flow_starts_and_bytes(fn)
    m = gurobipy.Model("fct")
    N = 8  # NUM_RACKS
    ps = 8  # packet switch
    STEP = int(1e3)
    end_time = int(2.5e6)
    no_circuit = 'no_circuit' in fn
    two_rings = 'two_rings' in fn

    V = list(xrange(N))
    E = []
    Capacities = {}
    if not no_circuit:
        for v in V:
            e = (v, (v+1) % N)
            E.append(e)
            if not two_rings:
                # .99 comes from 1 - (20us / 2000us)
                Capacities[e] = .99 * 80 * 1e3 * STEP / 8.0  # bytes in a STEP
            if two_rings:
                # .98 comes from 1 - (20us / 1000us)
                Capacities[e] = .98 * 40 * 1e3 * STEP / 8.0  # bytes in a STEP
                e = (v, (v+2) % N)
                E.append((v, (v+2) % N))
                Capacities[e] = .98 * 40 * 1e3 * STEP / 8.0  # bytes in a STEP
    for v in V:
        for v2 in V:
            if v != v2 and (v, v2) not in E:
                E.append((v, ps))
                E.append((ps, v2))
                Capacities[(v, ps)] = 10 * 1e3 * STEP / 8.0  # bytes in a STEP
                Capacities[(ps, v2)] = 10 * 1e3 * STEP / 8.0  # bytes in a STEP
    V.append(ps)

    print time.time(), 'adding variables'
    F = defaultdict(lambda: defaultdict(list))  # variables
    coeffs = []
    for i, f in enumerate(flows):
        for t in xrange(0, end_time, STEP):
            if t < f['start']:  # can't use flow at this time step
                continue
            for e in E:
                # coefficient grows quadratically away from start
                coeffs.append((t - f['start'])**2)
    print time.time(), 'adding to model'
    U = m.addVars(len(coeffs), lb=0, vtype=gurobipy.GRB.INTEGER, obj=coeffs)
    print time.time(), 'dict-izing'
    u = 0
    for i, f in enumerate(flows):
        for t in xrange(0, end_time, STEP):
            if t < f['start']:
                continue
            for e in E:
                # bytes sent on edge by flow at timestep
                F[i][t].append(U[u])
                u += 1
    print time.time(), 'done'

    m.modelSense = gurobipy.GRB.MINIMIZE
    print time.time(), 'updating model'
    m.update()

    print time.time(), 'starting constraints'
    # capacity constraint
    for t in xrange(0, end_time, STEP):
        used = {}
        for j, e in enumerate(E):
            used[e] = [F[i][t][j] for i, f in enumerate(flows)
                       if f['start'] < t]
        m.addConstrs(gurobipy.quicksum(used[e]) <= Capacities[e] for e in E)
    print time.time(), 'done capacity constraints'

    # transit constraint
    for i, f in enumerate(flows):
        for v in V:
            if v == f['src'] or v == f['dst']:
                continue
            for t in xrange(0, end_time, STEP):
                if t < f['start']:
                    continue
                # f entering v
                enter_v = [F[i][t][j] for j, e in enumerate(E) if e[1] == v]
                # f leaving v
                leave_v = [F[i][t][j] for j, e in enumerate(E) if e[0] == v]
                m.addConstr(gurobipy.quicksum(enter_v) == gurobipy.quicksum(leave_v))
    print time.time(), 'done transit constraints'

    # source must send all bytes
    for i, f in enumerate(flows):
        enter_s = []
        leave_s = []
        for t in xrange(0, end_time, STEP):
            if t < f['start']:
                continue
            # f entering source
            enter_s += [F[i][t][j] for j, e in enumerate(E)
                        if e[1] == f['src']]
            # f leaving source
            leave_s += [F[i][t][j] for j, e in enumerate(E)
                        if e[0] == f['src']]
        m.addConstr(gurobipy.quicksum(leave_s) - gurobipy.quicksum(enter_s) ==
                    f['bytes'])
    print time.time(), 'done source constraints'

    # dest must sink all bytes
    for i, f in enumerate(flows):
        enter_d = []
        leave_d = []
        for t in xrange(0, end_time, STEP):
            if t < f['start']:
                continue
            # f entering dest
            enter_d += [F[i][t][j] for j, e in enumerate(E)
                        if e[1] == f['dst']]
            # f leaving dest
            leave_d += [F[i][t][j] for j, e in enumerate(E)
                        if e[0] == f['dst']]
        m.addConstr(gurobipy.quicksum(enter_d) - gurobipy.quicksum(leave_d) ==
                    f['bytes'])
    print time.time(), 'done dest constraints'

    print time.time(), 'optimizing'
    m.optimize()

    print time.time(), 'getting results'
    for i, f in enumerate(flows):
        last = -1
        for t in xrange(0, end_time, STEP):
            if t < f['start']:
                continue
            for j, e in enumerate(E):
                if F[i][t][j].x:
                    last = t
        f['fct'] = last - f['start']
        f['tput'] = f['bytes'] / float(f['fct'])
    return flows

if __name__ == '__main__':
    print sys.argv[1] + '/*-normal-*-QUEUE-False-*-reno-*-flowgrind.txt'
    for fn in glob.glob(sys.argv[1] + '/*-normal-*-'
                        'QUEUE-False-*-reno-*flowgrind.txt'):
        flows = get_optimal_fct(fn)
        print flows
