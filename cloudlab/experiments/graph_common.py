import os
import tarfile
import glob
import math
import numpy as np


def get_ping_from_file(fn):
    lats = {}
    sn = 0
    ping_file = list(open(fn))
    if len(ping_file) < 2 or 'bad: timeout' in ping_file[0] \
       or 'bad RC' in ping_file[0]:
        print 'bad timeout'
        return lats
    for line_space in ping_file[1:]:
        line = line_space.strip()
        if 'bad: timeout' in line:
            break
        if not line:
            break
        if line[0] == '-':
            break
        if 'Warning' in line:
            continue
        sn = int(line.split('icmp_seq=')[1].split()[0])
        lat = float(line.split('time=')[1].split()[0])
        lats[sn] = lat * 1000.0  # usecs
    if lats:
        max_sn = max(lats.keys())
        lats = [l for (s, l) in lats.items() if s > 5 and s < max_sn - 5]
    else:
        print 'ping error'
    return lats

    
def get_iperf3_tput_from_file(fn):
    tps = []
    iperf3_file = list(open(fn))
    if len(iperf3_file) < 4 or 'bad: timeout' in iperf3_file[0]:
        print 'bad timeout'
        return [0]
    for line in iperf3_file[3:]:
        if 'bad: timeout' in line:
            print 'bad timeout'
            break
        if line[0] == '-':
            break
        # if 'sender' not in line:
        #     continue
        l = line.split(']')[1].strip().split()
        tp = float(l[4])
        units = l[5]
        if units == 'Gbits/sec':
            tp = tp
        elif units == 'Mbits/sec':
            tp = tp / 1000.0
        elif units == 'Kbits/sec':
            tp = tp / 1000000.0
        elif units == 'bits/sec':
            tp = tp / 1000000000.0
        tps.append(tp)
    tps = tps[1:-1]  # throw away first and last measurement
    if tps:
        return tps
    else:
        return 0


def untar(fn):
    TMP = "/tmp/sdrt/%s" % (os.path.basename(fn).split('.tar.gz')[0])
    if not os.path.exists(TMP):
        os.makedirs(TMP)
        dir = os.getcwd()
        os.chdir(TMP)
        tar = tarfile.open(dir + "/" + os.path.basename(fn))
        tar.extractall()
        tar.close()
        os.chdir(dir)


def get_tput_and_lat(pattern, ts, c):
    untar('%s-%s.tar.gz' % (ts, pattern))
    print c
    NUM_RACKS = 8  # grab num_racks from somewhere...
    tputs = [[] for x in xrange(NUM_RACKS+1)]
    tputs_down = [[] for x in xrange(NUM_RACKS+1)]
    pings = [[] for x in xrange(NUM_RACKS+1)]
    tail_pings = []
    fn_format = '/tmp/sdrt/%s-%s/%s-%s-%s-%d-%s-%s-*' % (
        ts, pattern, ts, pattern, c['type'], c['buffer_size'],
        c['traffic_source'], c['queue_resize'])
    fn_format += '-%s.txt'
    ping_files = glob.glob(fn_format % 'ping')
    iperf3_files = glob.glob(fn_format % 'iperf3')
    for ip3fn in iperf3_files:
        src, dst = ip3fn.replace(fn_format.split('*')[0],
                                 '').split('-iperf3')[0].split('-')
        src_rack = int(src[1])
        dst_rack = int(dst[1])
        file_iperf_avg = np.average(get_iperf3_tput_from_file(ip3fn))
        tputs[src_rack].append(file_iperf_avg)
        tputs_down[dst_rack].append(file_iperf_avg)
    for pfn in ping_files:
        src, dst = pfn.replace(fn_format.split('*')[0],
                               '').split('-ping')[0].split('-')
        src_rack = int(src[1])
        dst_rack = int(dst[1])
        p = get_ping_from_file(pfn)
        if p:
            pings[src_rack].append(np.median(p))
            tail_pings += p
    for rack in xrange(NUM_RACKS+1):
        tputs[rack] = (sum(tputs[rack]) / 80.0) * 100
        tputs_down[rack] = (sum(tputs_down[rack]) / 80.0) * 100
        pings[rack] = np.median(pings[rack])
    tputs = filter(lambda a: a, tputs)
    tputs_down = filter(lambda a: a, tputs_down)
    pings = filter(lambda a: not math.isnan(a), pings)
    # tputs = (np.median(tputs), np.std(tputs))
    # tputs = (sum(tputs), np.std(tputs))
    tputs = (np.average(tputs), np.std(tputs))
    tputs_down = (np.average(tputs_down), np.std(tputs_down))
    pings = (np.median(pings), np.std(pings))
    if tail_pings:
        tail_pings = (np.percentile(tail_pings, 90), np.percentile(tail_pings, 99))
    else:
        tail_pings = (np.nan, np.nan)
    return tputs, pings, tputs_down, tail_pings
