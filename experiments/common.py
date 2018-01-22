#!/usr/bin/python

import time
import threading
import sys
import os
import tarfile
import socket
import rpyc
import numpy as np
import click_common
from subprocess import call, PIPE, STDOUT, Popen
from globals import NUM_RACKS, HOSTS_PER_RACK, TIMESTAMP, SCRIPT, \
    EXPERIMENTS, PHYSICAL_NODES, RPYC_CONNECTIONS, RPYC_PORT, CIRCUIT_BW, \
    PACKET_BW

DATA_NET = 1
CONTROL_NET = 2

THREADS = []
THREAD_LOCK = threading.Lock()

CDFs = {
    'DCTCP': [
        (0, 0),
        (10000, 0.15),
        (20000, 0.2),
        (30000, 0.3),
        (50000, 0.4),
        (80000, 0.53),
        (200000, 0.6),
        (1e+06, 0.7),
        (2e+06, 0.8),
        (5e+06, 0.9),
        (1e+07, 0.97),
        (3e+07, 1),
    ],

    'VL2': [
        (0, 0),
        (180, 0.1),
        (216, 0.2),
        (560, 0.3),
        (900, 0.4),
        (1100, 0.5),
        (1870, 0.6),
        (3160, 0.7),
        (10000, 0.8),
        (400000, 0.9),
        (3.16e+06, 0.95),
        (1e+08, 0.98),
        (1e+09, 1),
    ],

    'FB': [
        (0, 0),
        (3, 0.28),
        (10, 0.32),
        (12, 0.44),
        (75, 0.5),
        (250, 0.75),
        (350, 0.9),
        (1000, 0.965),
        (2515, 0.98),
        (6326, 0.99),
        (10032, 0.995),
        (15910, 0.997),
        (23009, 0.9992),
        (24094, 0.9996),
        (25000, 0.9999),
        (100000, 0.99999),
        (1e+06, 1),
    ],
}

FANOUT = [
    (1, 1.0),
    # (1, .50),
    # (2, .30),
    # (4, .20),
]


##
# Experiment commands
##
def initializeExperiment(adu=False, hadoop=False):
    print '--- starting experiment...'
    print '--- clearing local arp...'
    call([os.path.expanduser('~/sdrt/cloudlab/arp_clear.sh')])
    print '--- done...'

    print '--- populating physical hosts...'

    del PHYSICAL_NODES[:]  # clear in place
    PHYSICAL_NODES.append('')
    for i in xrange(1, NUM_RACKS+1):
        PHYSICAL_NODES.append('host%d' % i)
    print '--- done...'

    print '--- connecting to rpycd...'
    connect_all_rpyc_daemon()
    print '--- done...'

    print '--- setting CC to reno...'
    click_common.setCC('reno', adu, hadoop)
    print '--- done...'

    print '--- launching flowgrindd...'
    launch_all_flowgrindd(adu, hadoop)
    print '--- done...'

    click_common.initializeClickControl()

    print '--- setting default click buffer sizes and traffic sources...'
    click_common.setConfig({})
    print '--- done...'
    time.sleep(2)
    print '--- done starting experiment...'
    print
    print


def finishExperiment():
    print '--- finishing experiment...'
    print '--- closing final log...'
    click_common.setLog('/tmp/hslog.log')
    print '--- done...'
    print '--- tarring output...'
    tarExperiment()
    print '--- done...'
    print '--- experiment finished'
    print TIMESTAMP


def tarExperiment():
    tar = tarfile.open("%s-%s.tar.gz" % (TIMESTAMP, SCRIPT), "w:gz")
    for e in EXPERIMENTS:
        tar.add(e)
    tar.close()

    for e in EXPERIMENTS:
        if 'tmp' not in e:
            os.remove(e)


##
# rpyc_daemon
##
def connect_all_rpyc_daemon():
    bad_hosts = []
    for phost in PHYSICAL_NODES[1:]:
        try:
            if phost not in RPYC_CONNECTIONS:
                RPYC_CONNECTIONS[phost] = rpyc.connect(phost, RPYC_PORT)
        except:
            print 'could not connect to ' + phost
            bad_hosts.append(phost)
    map(lambda x: PHYSICAL_NODES.remove(x), bad_hosts)


##
# flowgrind
##
def launch_flowgrindd(phost, adu, hadoop):
    if hadoop:
        if adu:
            RPYC_CONNECTIONS[phost].root.hadoop_adu()
        else:
            RPYC_CONNECTIONS[phost].root.hadoop()
    else:
        if adu:
            RPYC_CONNECTIONS[phost].root.flowgrindd_adu()
        else:
            RPYC_CONNECTIONS[phost].root.flowgrindd()


def launch_all_flowgrindd(adu, hadoop):
    ts = []
    for phost in PHYSICAL_NODES[1:]:
        ts.append(threading.Thread(target=launch_flowgrindd,
                                   args=(phost, adu, hadoop)))
        ts[-1].start()
    map(lambda t: t.join(), ts)
    if not hadoop:
        for r in xrange(1, NUM_RACKS+1):
           for h in xrange(1, HOSTS_PER_RACK+1):
               ip = '10.2.%d.%d' % (r, h)
               port = 5999
               sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
               while sock.connect_ex((ip, port)):
                   time.sleep(1)
               sock.close()


def get_flowgrind_host(h):
    if len(h) == 2:
        return '10.%s.10.%s/10.%s.10.%s' % (DATA_NET, h[1],
                                            CONTROL_NET, h[1])
    else:
        return '10.%s.%s.%s/10.%s.%s.%s' % (DATA_NET, h[1], h[2:],
                                            CONTROL_NET, h[1], h[2:])


def gen_empirical_flows(seed=92611, cdf_key='DCTCP'):
    np.random.seed(seed)
    cdf = CDFs[cdf_key]
    target_bw = 1/2.0 * \
        ((PACKET_BW + 1.0/NUM_RACKS * CIRCUIT_BW) / 8.0) / HOSTS_PER_RACK
    flows = []
    for r in xrange(1, NUM_RACKS+1):
        for h in xrange(1, HOSTS_PER_RACK+1):
            src = 'h%d%d' % (r, h)
            src_num = (r-1)*HOSTS_PER_RACK + (h-1)
            t = 0.0
            while t < 2.0:
                request_size = np.random.randint(60, 120)
                response_size = int(round(np.interp(np.random.random(),
                                                    zip(*cdf)[1],
                                                    zip(*cdf)[0])))
                response_size = min(response_size, target_bw)
                fout = np.random.choice(zip(*FANOUT)[0], p=zip(*FANOUT)[1])
                for i in xrange(fout):
                    dst_num = np.random.choice([x for x in
                                                xrange(NUM_RACKS * HOSTS_PER_RACK)
                                                if x != src_num])
                    dst = 'h%d%d' % ((dst_num / HOSTS_PER_RACK) + 1,
                                     (dst_num % HOSTS_PER_RACK) + 1)
                    flows.append({'src': src, 'dst': dst, 'start': t,
                                  # 'size': request_size,
                                  # 'response_size': max(response_size / fout, 1),
                                  'size': max(response_size / fout, 1),
                                  'single': True})
                t += response_size / target_bw
    print len(flows)
    return flows


def gen_big_and_small_flows(seed=92611):
    np.random.seed(seed)
    big_bw = 1/3.0 * CIRCUIT_BW / 8.0
    little_bw = 1/3.0 * PACKET_BW / 8.0 / NUM_RACKS
    big_nodes = [(1, 2), (2, 3), (3, 4), (4, 5),
                 (5, 6), (6, 7), (7, 8), (8, 1)]
    flows = []
    psize = 9000
    for s in xrange(1, NUM_RACKS+1):
        for d in xrange(1, NUM_RACKS+1):
            t = 0.0
            while t < 2.0:
                src = 'h%d%d' % (s, np.random.randint(1, HOSTS_PER_RACK+1))
                dst = 'h%d%d' % (d, np.random.randint(1, HOSTS_PER_RACK+1))
                size = np.random.randint(10 * psize, 100 * psize)
                if (s, d) in big_nodes:
                    size = np.random.randint(1000 * psize, 10000 * psize)
                flows.append({'src': src, 'dst': dst, 'start': t,
                              'size': size,
                              'response_size': 60,
                              'single': True})
                target_bw = big_bw if (s, d) in big_nodes else little_bw
                t += size / target_bw
    print len(flows)
    return flows


def gen_big_and_small_two_rings_flows(seed=92611):
    np.random.seed(seed)
    big_bw = 1/6.0 * CIRCUIT_BW / 8.0
    little_bw = 1/3.0 * PACKET_BW / 8.0 / NUM_RACKS
    big_nodes = [(1, 2), (2, 3), (3, 4), (4, 5),
                 (5, 6), (6, 7), (7, 8), (8, 1),
                 (1, 3), (2, 4), (3, 5), (4, 6),
                 (5, 7), (6, 8), (7, 1), (8, 2),]
    flows = []
    psize = 9000
    for s in xrange(1, NUM_RACKS+1):
        for d in xrange(1, NUM_RACKS+1):
            t = 0.0
            while t < 2.0:
                src = 'h%d%d' % (s, np.random.randint(1, HOSTS_PER_RACK+1))
                dst = 'h%d%d' % (d, np.random.randint(1, HOSTS_PER_RACK+1))
                size = np.random.randint(10 * psize, 100 * psize)
                if (s, d) in big_nodes:
                    size = np.random.randint(1000 * psize, 10000 * psize)
                flows.append({'src': src, 'dst': dst, 'start': t,
                              'size': size,
                              'response_size': 60,
                              'single': True})
                target_bw = big_bw if (s, d) in big_nodes else little_bw
                t += size / target_bw
    print len(flows)
    return flows


def flowgrind(settings):
    flows = []
    if 'empirical' in settings:
        flows = gen_empirical_flows(cdf_key=settings['empirical'])
    if 'big_and_small' in settings:
        flows = gen_big_and_small_flows()
    if 'big_and_small_two_rings' in settings:
        flows = gen_big_and_small_two_rings_flows()
    else:
        if 'flows' in settings:
            for f in settings['flows']:
                if f['src'][0] == 'r' and f['dst'][0] == 'r':
                    s = int(f['src'][1])
                    d = int(f['dst'][1])
                    for i in xrange(1, HOSTS_PER_RACK+1):
                        fl = dict(f)
                        fl['src'] = 'h%d%d' % (s, i)
                        fl['dst'] = 'h%d%d' % (d, i)
                        flows.append(fl)
                    # flows.append({'src': 'h1', 'dst': 'h2'})
                else:
                    flows.append(f)
    cmd = '-n %s ' % len(flows)
    for i, f in enumerate(flows):
        if 'time' not in f:
            f['time'] = 2.0
        if 'start' not in f:
            f['start'] = 0
        if 'size' not in f:
            f['size'] = 8948
        cmd += '-F %d ' % (i)
        if 'fg_report_interval' in settings:
            cmd += '-i %f ' % (settings['fg_report_interval'])
        else:
            cmd += '-Q -i 2 '
        cmd += '-Hs=%s,d=%s -Ts=%f -Ys=%f -Gs=q:C:%d ' % \
            (get_flowgrind_host(f['src']), get_flowgrind_host(f['dst']),
             f['time'], f['start'], f['size'])
        if 'response_size' in f:
            cmd += '-Gs=p:C:%d ' % f['response_size']
        if 'single' in f:
            cmd += '-Z 1 '
    cmd += '-I '
    fg_config = click_common.FN_FORMAT % ('flowgrind.config')
    fp = open(fg_config, 'w')
    fp.write(cmd)
    fp.close()
    cmd = 'flowgrind --configure %s' % fg_config
    print cmd
    EXPERIMENTS.append(fg_config)
    fn = click_common.FN_FORMAT % ('flowgrind')
    print fn
    runWriteFile(cmd, fn)
    counters_fn = click_common.FN_FORMAT % ('flowgrind.counters')
    fp = open(counters_fn, 'w')
    counter_data = click_common.getCounters()
    fp.write(str(counter_data))
    fp.close()
    EXPERIMENTS.append(counters_fn)


##
# Running shell commands
##
def run(cmd):
    def preexec():  # don't forward signals
        os.setpgrp()

    out = ""
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=STDOUT,
              preexec_fn=preexec)
    while True:
        line = p.stdout.readline()  # this will block
        if not line:
            break
        # sys.stdout.write(line)
        out += line
    rc = p.poll()
    while rc is None:
        rc = p.poll()
    if rc != 0:
        raise Exception("subprocess.CalledProcessError: Command '%s'"
                        "returned non-zero exit status %s\n"
                        "output was: %s" % (cmd, rc, out))
    return (rc, out)


def runWriteFile(cmd, fn):
    EXPERIMENTS.append(fn)
    try:
        out = run(cmd)[1]
    except Exception, e:
        print e
        out = str(e)
    if fn:
        f = open(fn, 'w')
        f.write(out)
        f.close()
