#!/usr/bin/env python

import sys
import glob
import numpy as np

from struct import unpack
from collections import defaultdict

percentiles = [25, 50, 75, 99, 99.9, 99.99, 99.999, 100]
RTT = 0.001200
CIRCUIT_BW = 4  # without TDF


def get_tput_and_dur(fn):
    flows = defaultdict(list)
    sizes = {}
    for l in open(fn.split('.txt')[0] + '.config.txt'):
        l = l.split('-F')[1:]
        for x in l:
            id = int(x.strip().split()[0])
            bytes = int(x.strip().split('-Gs=q:C:')[1].split()[0])
            sizes[id] = bytes
    for l in open(fn):
        if 'seed' in l:
            id = int(l[4:].strip().split()[0])
            rack = int(l.split('/')[0].split()[-1].split('.')[2])
            flows[id].append(rack)
            if l.split(":")[0][-1] == 'S':
                time = float(l.split('duration = ')[1].split('/')[0]) * 1000000
                tp = float(l.split('through = ')[1].split('/')[0]) / 1000.0
            else:
                flows[id].append(time)
                flows[id].append(tp)
                flows[id].append(sizes[id])

    for f in flows.keys():
        if flows[f][3] == float('inf'):
            del flows[f]
    tps = [f[3] for f in flows.values()]
    durs = [f[2] for f in flows.values()]
    bytes = [f[4] for f in flows.values()]

    print len(tps), len(durs), len(bytes)

    return tps, durs, bytes


def msg_from_file(filename, chunksize=112):
    with open(filename, "rb") as f:
        while True:
            chunk = f.read(chunksize)
            if chunk:
                yield chunk
            else:
                break


def get_seq_data(fn):
    circuit_starts = defaultdict(list)
    circuit_ends = defaultdict(list)
    flows = defaultdict(list)
    for msg in msg_from_file(fn):
        (t, ts, lat, src, dst, data) = unpack('i32siii64s', msg)
        bytes = unpack('!H', data[2:4])[0]
        a, b, c, d = [ord(x) for x in data[12:16]]
        sender = '%d.%d.%d.%d' % (a, b, c, d)
        a, b, c, d = [ord(x) for x in data[16:20]]
        recv = '%d.%d.%d.%d' % (a, b, c, d)
        proto = ord(data[9])
        ihl = (ord(data[0]) & 0xF) * 4
        sport = 0
        dport = 0
        seq = 0
        if proto == 6:  # TCP
            sport = unpack('!H', data[ihl:ihl+2])[0]
            dport = unpack('!H', data[ihl+2:ihl+4])[0]
            seq = unpack('!I', data[ihl+4:ihl+8])[0]

        for i in xrange(len(ts)):
            if ord(ts[i]) == 0:
                break
        ts = float(ts[:i]) / 20.0

        if t == 1 or t == 2:  # starting or closing
            sr = (src, dst)
            if t == 1:  # starting
                circuit_starts[sr].append(ts)
            if t == 2:  # closing
                if not circuit_starts[sr]:
                    continue
                circuit_ends[sr].append(ts)
            continue

        flow = (sender, recv, proto, sport, dport)
        flows[flow].append((ts, seq))
        if bytes < 100:
            continue

    day_lens = []
    week_lens = []
    starts = []
    ends = []
    next_starts = []
    next_ends = []
    for i in xrange(1, len(circuit_starts[(1, 2)])-1):
        prev_end = circuit_ends[(1, 2)][i-1]
        curr = circuit_starts[(1, 2)][i]
        curr_end = circuit_ends[(1, 2)][i]
        next = circuit_starts[(1, 2)][i+1]
        if i+1 >= len(circuit_ends[(1, 2)]):
            continue
        next_end = circuit_ends[(1, 2)][i+1]
        day_lens.append((curr_end - curr)*1e6)
        week_lens.append((next - curr)*1e6)
        starts.append((curr - prev_end)*1e6)
        ends.append((curr_end - prev_end)*1e6)
        next_starts.append((next - prev_end)*1e6)
        next_ends.append((next_end - prev_end)*1e6)
    out_start = np.average(starts[5:-5])
    out_end = np.average(ends[5:-5])
    out_next_start = np.average(next_starts[5:-5])
    out_next_end = np.average(next_ends[5:-5])
    day_lens = day_lens[5:-5]
    week_lens = week_lens[5:-5]
    print 'circuit day avg and std dev', np.average(day_lens), np.std(day_lens)
    print 'week avg and std dev', np.average(week_lens), np.std(week_lens)
    print out_start, out_end, out_next_start, out_next_end

    if len(circuit_starts[(1, 2)]) < 50:
        ts_start = flows.values()[0][0][0]
        ts_end = flows.values()[0][-1][0]
        for f in flows:
            fstart = flows[f][0][0]
            fend = flows[f][-1][0]
            if fstart < ts_start:
                ts_start = fstart
            if fend > ts_end:
                ts_end = fend
        circuit_starts[(1, 2)] = np.arange(ts_start - 0.002,
                                           ts_end + 0.002, 0.002)
        circuit_ends[(1, 2)] = np.arange(ts_start, ts_end + 0.004, 0.002)

    print len(flows)
    results = defaultdict(list)
    for f in flows.keys():
        if '10.1.2.' in f[0]:
            continue
        print f
        chunks = []
        last = 0
        print len(circuit_starts[(1, 2)])
        bad_windows = 0
        first_ts = flows[f][0][0]
        last_ts = flows[f][-1][0]
        for i in xrange(1, len(circuit_starts[(1, 2)])-2):
            prev_end = circuit_ends[(1, 2)][i-1]
            curr = circuit_starts[(1, 2)][i]
            curr_end = circuit_ends[(1, 2)][i]
            if curr_end < first_ts:
                continue
            if curr > last_ts:
                continue
            next = circuit_starts[(1, 2)][i+1]
            if i+1 >= len(circuit_ends[(1, 2)]):
                continue
            next_end = circuit_ends[(1, 2)][i+1]
            next_next = circuit_starts[(1, 2)][i+2]
            out = []
            first = -1
            for i in xrange(last, len(flows[f])):
                (ts, seq) = flows[f][i]
                if ts > next_next:
                    break
                if ts >= prev_end:
                    if first == -1:
                        first = seq
                    out.append(((ts - prev_end)*1e6, seq - first))
                if ts < curr_end:
                    last = i
            if not out:
                bad_windows += 1
                out = [(0, 0), (4000, 0)]
            wraparound = False
            for ts, seq in out:
                if seq < -1e8 or seq > 1e8:
                    wraparound = True
            if not wraparound:
                chunks.append(np.interp(xrange(4000),
                                        zip(*out)[0],
                                        zip(*out)[1]))
        print len(chunks)
        unzipped = zip(*chunks)
        results[f] = [np.average(q) for q in unzipped]
        print 'bad windows', bad_windows
    unzipped = zip(*results.values())
    results = [np.average(q) for q in unzipped]
    return results, (out_start, out_end, out_next_start, out_next_end)


def get_tput_and_lat(fn):
    latencies = []
    latencies_circuit = []
    latencies_packet = []
    throughputs = defaultdict(int)
    circuit_bytes = defaultdict(int)
    packet_bytes = defaultdict(int)
    flow_start = {}
    flow_end = {}
    number_circuit_ups = defaultdict(int)
    circuit_starts = defaultdict(list)
    most_recent_circuit_up = defaultdict(int)
    bytes_in_rtt = defaultdict(lambda: defaultdict(int))
    for msg in msg_from_file(fn):
        (t, ts, lat, src, dst, data) = unpack('i32siii64s', msg)
        bytes = unpack('!H', data[2:4])[0]
        circuit = ord(data[1]) & 0x1
        sender = ord(data[14])
        recv = ord(data[18])
        for i in xrange(len(ts)):
            if ord(ts[i]) == 0:
                break
        ts = float(ts[:i])
        if t == 1 or t == 2:  # starting or closing
            sr = (src, dst)
            if t == 1:  # starting
                most_recent_circuit_up[sr] = ts
            if t == 2:  # closing
                circuit_starts[sr].append(ts / 20.0)
                number_circuit_ups[sr] += 1
            continue

        latency = float(lat)
        sr = (sender, recv)
        if bytes < 100:
            continue
        if sr not in flow_start:
            flow_start[sr] = ts / 20.0  # TDF

        if circuit:
            which_rtt = int((ts - most_recent_circuit_up[sr] - 0.5*RTT) / RTT)
            bytes_in_rtt[sr][which_rtt] += bytes
            circuit_bytes[sr] += bytes
        else:
            packet_bytes[sr] += bytes

        throughputs[sr] += bytes
        flow_end[sr] = ts / 20.0  # TDF
        if bytes > 1000:
            latencies.append(latency)
            if circuit:
                latencies_circuit.append(latency)
            else:
                latencies_packet.append(latency)
    lat = zip(percentiles, map(lambda x: np.percentile(latencies, x),
                               percentiles))
    latc = [(p, 0) for p in percentiles]
    if latencies_circuit:
        latc = zip(percentiles, map(lambda x: np.percentile(
            latencies_circuit, x), percentiles))
    latp = zip(percentiles, map(lambda x: np.percentile(latencies_packet, x),
                                percentiles))
    tp = {}
    b = defaultdict(dict)
    p = {}
    c = {}
    for sr in flow_start:
        total_time = flow_end[sr] - flow_start[sr]
        tp[sr] = throughputs[sr] / total_time
        tp[sr] *= 8  # bytes to bits
        tp[sr] /= 1000000000  # bits to gbits

        p[sr] = (packet_bytes[sr] / total_time) * 8 / 10**9
        c[sr] = (circuit_bytes[sr] / total_time) * 8 / 10**9

        n = 0
        for ts in circuit_starts[sr]:
            if ts >= flow_start[sr] and ts <= flow_end[sr]:
                n += 1
        print n, number_circuit_ups[sr]
        max_bytes = n * RTT * (CIRCUIT_BW * 10**9 / 8.0)
        for i, r in sorted(bytes_in_rtt[sr].items()):
            b[sr][i] = (r / max_bytes) * 100

    print
    print fn
    print tp
    print lat
    print sorted(b.items())
    print p
    print c
    return tp, (lat, latc, latp), p, c, b, \
        sum(circuit_bytes.values()), sum(packet_bytes.values())

if __name__ == "__main__":
    fns = sys.argv[1]
    if 'tmp' not in fns:
        fns += '/tmp/*'
    for fn in glob.glob(fns):
        get_tput_and_lat(fn)
