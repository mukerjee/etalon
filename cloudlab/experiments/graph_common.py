import os
import tarfile
import glob
import numpy as np


def get_ping_from_file(fn):
    ping_file = list(open(fn))[1:]
    lats = {}
    sn = 0
    for line_space in ping_file:
        line = line_space.strip()
        if not line:
            break
        if line[0] == '-':
            break
        if 'Warning' in line:
            continue
        sn = int(line.split('icmp_seq=')[1].split()[0])
        lat = float(line.split('time=')[1].split()[0])
        lats[sn] = lat * 1000.0  # usecs
    max_sn = max(lats.keys())
    lats = [l for (s, l) in lats.items() if s > 5 and s < max_sn - 5]
    return lats

    
def get_iperf3_tput_from_file(fn):
    iperf3_file = list(open(fn))[3:]
    tps = []
    for line in iperf3_file:
        if line[0] == '-':
            break
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
    return tps


def untar(fn):
    TMP = "/tmp/sdrt"
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
    tputs = []
    pings = []
    fn_format = '%s-%s-%s-%d-%s-%s-*-%s.txt' % (ts, pattern, c['type'],
                                                c['buffer_size'],
                                                c['traffic_source'],
                                                c['queue_resize'])
    
    ping_files = glob.glob('/tmp/sdrt/' + fn_format % 'ping')
    iperf3_files = glob.glob('/tmp/sdrt/' + fn_format % 'iperf3')
    for ip3fn in iperf3_files:
        tputs.append(get_iperf3_tput_from_file(ip3fn))
    for pfn in ping_files:
        pings.append(get_ping_from_file(pfn))
    tputs = [np.median(host_tput) for host_tput in tputs]
    tputs = (np.median(tputs), np.var(tputs))
    pings = [np.median(host_ping) for host_ping in pings]
    pings = (np.median(pings), np.var(tputs))
    return tputs, pings
