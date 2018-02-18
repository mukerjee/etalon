#!/usr/bin/env python

import copy
import glob
import socket
import numpy as np

from struct import unpack
from collections import defaultdict

percentiles = [25, 50, 75, 99, 99.9, 99.99, 99.999, 100]
RTT = 0.001200
CIRCUIT_BW = 4  # without TDF
TDF = 20.0
bin_size = 1


def parse_flowgrind_config(fn):
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
    seen = defaultdict(list)
    for msg in msg_from_file(fn):
        (t, ts, lat, src, dst, data) = unpack('i32siii64s', msg)
        ip_bytes = unpack('!H', data[2:4])[0]
        sender = socket.inet_ntoa(data[12:16])
        recv = socket.inet_ntoa(data[16:20])
        proto = ord(data[9])
        ihl = (ord(data[0]) & 0xF) * 4
        sport = 0
        dport = 0
        seq = 0
        thl = 0
        if proto == 6:  # TCP
            sport = unpack('!H', data[ihl:ihl+2])[0]
            dport = unpack('!H', data[ihl+2:ihl+4])[0]
            seq = unpack('!I', data[ihl+4:ihl+8])[0]
            thl = (ord(data[ihl+12]) >> 4) * 4
        bytes = ip_bytes - ihl - thl

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
        if not flows[flow]:
            flows[flow].append((ts, seq, bytes))
        else:
            last_ts, last_seq, last_bytes = flows[flow][-1]
            if abs(last_seq + last_bytes - seq) < 2:
                updated = True
                while updated:
                    updated = False
                    for prev_seen in seen[flow]:
                        prev_seq = prev_seen[1]
                        prev_bytes = prev_seen[2]
                        if abs(seq + bytes - prev_seq) < 2:
                            seq = prev_seq
                            bytes = prev_bytes
                            seen[flow].remove(prev_seen)
                            updated = True
                            break
                flows[flow].append((ts, seq, bytes))
            else:
                seen[flow].append((ts, seq, bytes))

    day_lens = []
    week_lens = []
    starts = []
    ends = []
    next_starts = []
    next_ends = []
    next_next_starts = []
    next_next_ends = []
    sr = (1, 2)
    for i in xrange(1, len(circuit_starts[sr])-2):
        prev_end = circuit_ends[sr][i-1]
        curr = circuit_starts[sr][i]
        curr_end = circuit_ends[sr][i]
        next = circuit_starts[sr][i+1]
        if i+1 >= len(circuit_ends[sr]):
            continue
        next_end = circuit_ends[sr][i+1]
        next_next = circuit_starts[sr][i+2]
        if i+2 >= len(circuit_ends[sr]):
            continue
        next_next_end = circuit_ends[sr][i+2]
        day_lens.append((curr_end - curr)*1e6)
        week_lens.append((next - curr)*1e6)
        starts.append((curr - prev_end)*1e6)
        ends.append((curr_end - prev_end)*1e6)
        next_starts.append((next - prev_end)*1e6)
        next_ends.append((next_end - prev_end)*1e6)
        next_next_starts.append((next_next - prev_end)*1e6)
        next_next_ends.append((next_next_end - prev_end)*1e6)
    out_start = np.average(starts[5:-5])
    out_end = np.average(ends[5:-5])
    out_next_start = np.average(next_starts[5:-5])
    out_next_end = np.average(next_ends[5:-5])
    out_next_next_start = np.average(next_next_starts[5:-5])
    out_next_next_end = np.average(next_next_ends[5:-5])
    day_lens = day_lens[5:-5]
    week_lens = week_lens[5:-5]
    print 'circuit day avg and std dev', np.average(day_lens), np.std(day_lens)
    print 'week avg and std dev', np.average(week_lens), np.std(week_lens)
    print out_start, out_end, out_next_start, out_next_end

    if len(circuit_starts[sr]) < 50:
        ts_start = flows.values()[0][0][0]
        ts_end = flows.values()[0][-1][0]
        for f in flows:
            fstart = flows[f][0][0]
            fend = flows[f][-1][0]
            if fstart < ts_start:
                ts_start = fstart
            if fend > ts_end:
                ts_end = fend
        circuit_starts[sr] = np.arange(ts_start - 0.002,
                                       ts_end + 0.002, 0.002)
        circuit_ends[sr] = np.arange(ts_start, ts_end + 0.004, 0.002)

    print len(flows)
    results = defaultdict(list)
    for f in flows.keys():
        if '10.1.2.' in f[0]:
            continue
        print f
        chunks = []
        last = 0
        print len(circuit_starts[sr])
        bad_windows = 0
        first_ts = flows[f][0][0]
        last_ts = flows[f][-1][0]
        for i in xrange(1, len(circuit_starts[sr])-2):
            prev_end = circuit_ends[sr][i-1]
            curr = circuit_starts[sr][i]
            curr_end = circuit_ends[sr][i]
            if curr_end < first_ts:
                continue
            if curr > last_ts:
                continue
            next = circuit_starts[sr][i+1]
            if i+1 >= len(circuit_ends[sr]):
                continue
            next_end = circuit_ends[sr][i+1]
            next_next = circuit_starts[sr][i+2]
            if i+2 >= len(circuit_ends[sr]):
                continue
            next_next_end = circuit_ends[sr][i+2]
            out = []
            first = -1
            timing_offset = 30e-6
            for i in xrange(last, len(flows[f])):
                (ts, seq, _) = flows[f][i]
                if ts > next_next_end + timing_offset:
                    break
                if ts >= prev_end + timing_offset:
                    if first == -1:
                        first = seq
                    out.append(((ts - prev_end - timing_offset)*1e6,
                                seq - first))
                if ts < curr_end + timing_offset:
                    last = i
            if not out:
                bad_windows += 1
                out = [(0, 0), (4200, 0)]
            wraparound = False
            for ts, seq in out:
                if seq < -1e8 or seq > 1e8:
                    wraparound = True
            if not wraparound:
                chunks.append(np.interp(xrange(4200),
                                        zip(*out)[0],
                                        zip(*out)[1]))
        print len(chunks)
        unzipped = zip(*chunks)
        results[f] = [np.average(q) for q in unzipped]
        print 'bad windows', bad_windows
    results = {k: v for k, v in results.items() if v}
    unzipped = zip(*results.values())
    results = [np.average(q) for q in unzipped]
    return results, (out_start, out_end, out_next_start, out_next_end,
                     out_next_next_start, out_next_next_end)


def parse_packet_log(fn):
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
                circuit_starts[sr].append(ts / TDF)
                number_circuit_ups[sr] += 1
            continue

        latency = float(lat)
        sr = (sender, recv)
        if bytes < 100:
            continue
        if sr not in flow_start:
            flow_start[sr] = ts / TDF

        if circuit:
            which_rtt = int((ts - most_recent_circuit_up[sr] - 0.5*RTT) / RTT)
            bytes_in_rtt[sr][which_rtt] += bytes
            circuit_bytes[sr] += bytes
        else:
            packet_bytes[sr] += bytes

        throughputs[sr] += bytes
        flow_end[sr] = ts / TDF
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
        tp[sr] /= 1e9  # bits to gbits

        p[sr] = (packet_bytes[sr] / total_time) * 8 / 1e9
        c[sr] = (circuit_bytes[sr] / total_time) * 8 / 1e9

        n = 0
        for ts in circuit_starts[sr]:
            if ts >= flow_start[sr] and ts <= flow_end[sr]:
                n += 1
        max_bytes = n * RTT * (CIRCUIT_BW * 1e9 / 8.0)
        for i, r in sorted(bytes_in_rtt[sr].items()):
            b[sr][i] = (r / max_bytes) * 100

    print fn
    return tp, (lat, latc, latp), p, c, b, \
        sum(circuit_bytes.values()), sum(packet_bytes.values())


def parse_validation_log(folder, fns, packet):
    tp_out = {}
    for fn in fns:
        id_to_sr = {}
        for l in open(fn.split('.txt')[0] + '.config.txt'):
            l = l.split('-F')[1:]
            for x in l:
                id = int(x.strip().split()[0])
                s = int(x.strip().split('-Hs=10.1.')[1].split('.')[0])
                r = int(x.strip().split(',d=10.1.')[1].split('.')[0])
                id_to_sr[id] = (s, r)

        out_data = defaultdict(lambda: defaultdict(int))
        circuit = False if 'no_circuit' in fn else True
        for line in open(fn):
            if line[0] == 'S' and (line[1] == ' ' or line[1] == '1'):
                line = line[1:].strip()
                id = int(line.split()[0])
                ts_start = float(line.split()[1])
                ts_end = float(line.split()[2])
                curr_tp = float(line.split()[3]) * 1e6
                curr_bytes = (curr_tp / 8.0) * (ts_end - ts_start)
                out_data[id_to_sr[id]][ts_start] += curr_bytes
        for sr in out_data:
            out_data[sr] = sorted(out_data[sr].items())

        fn_ts = fn.split('/')[-1].split('-validation')[0]
        if circuit:
            packet_log_fn = glob.glob(folder +
                                      '/tmp/%s-validation-'
                                      'circuit-*-click.txt'
                                      % fn_ts)
        else:
            packet_log_fn = glob.glob(folder +
                                      '/tmp/%s-validation-'
                                      'no_circuit-*-click.txt'
                                      % fn_ts)
        if packet_log_fn:
            out_data = defaultdict(list)
            first_ts = -1
            for msg in msg_from_file(packet_log_fn[0]):
                (t, ts, lat, src, dst, data) = unpack('i32siii64s', msg)
                if t != 0:
                    continue
                bytes = unpack('!H', data[2:4])[0]
                sender = ord(data[14])
                recv = ord(data[18])
                sr = (sender, recv)
                for i in xrange(len(ts)):
                    if ord(ts[i]) == 0:
                        break
                ts = (float(ts[:i]) / TDF) * 1000
                if first_ts == -1:
                    first_ts = ts
                out_data[sr].append((ts - first_ts, bytes))

        print 'done parsing ' + fn
        tp = defaultdict(list)
        for sr in out_data:
            if packet_log_fn:
                curr = 0
                for i in xrange(0, 2000, bin_size):
                    curr_tp = 0
                    for j in xrange(curr, len(out_data[sr])):
                        ts, bytes = out_data[sr][j]
                        if ts >= i and ts < i+bin_size:
                            curr_tp += bytes
                        else:
                            curr = j
                            break
                    tp[sr].append(curr_tp * (1000.0 / bin_size) * 8.0 / 1e9)
            else:
                for i in xrange(0, 2000, bin_size):
                    curr = [b for ts, b in out_data[sr]
                            if ts >= i/1000.0 and ts < (i+bin_size) / 1000.0]
                    curr = sum(curr)
                    tp[sr].append((curr*8.0 / (bin_size / 1000.0) / 1e9))
        tp_out[fn] = copy.deepcopy(tp)
    tp = defaultdict(list)
    for sr in tp_out[fns[0]]:
        for i in xrange(0, 2000, bin_size):
            tp[sr].append(np.average([q[sr][i] for q in tp_out.values()]))
    print [len(tp[k]) for k in tp.keys()]
    x = [xrange(0, 2000, bin_size) for sr in tp.keys()]
    y = tp.values()

    means = []
    stds = []
    for data in y:
        for i, d in enumerate(data):
            if round(d) > 0:
                break
        means.append(np.mean(data[i:]))
        stds.append(np.std(data[i:]))
    print means
    print stds
    print np.mean(means)
    print np.mean(stds)


def parse_hdfs_logs(folder):
    data = []
    durations = []

    fn = folder + '/*-logs/hadoop*-datanode*.log'
    print fn
    logs = glob.glob(fn)

    for log in logs:
        for line in open(log):
            if 'clienttrace' in line and 'HDFS_WRITE' in line:
                bytes = int(line.split('bytes: ')[1].split()[0][:-1])
                if bytes > 1024 * 1024 * 99:
                    duration = (float(line.split(
                        "duration: ")[1].split()[0]) * 1e-6) / TDF
                    data.append((bytes / 1024. / 1024., duration))

    durations = sorted(zip(*data)[1])
    return durations


def parse_hdfs_throughput(folder):
    fn = folder + '/report/dfsioe/hadoop/bench.log'
    logs = glob.glob(fn)

    for log in logs:
        for line in open(log):
            if 'Number of files' in line:
                num_files = int(line.split('files: ')[1])
            if 'Throughput mb/sec:' in line:
                return (float(line.split('mb/sec:')[1]) *
                        num_files * 8 / 1024.0) * TDF
