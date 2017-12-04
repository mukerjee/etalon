#!/usr/bin/python

import time
import threading
import sys
import os
import tarfile
import rpyc
import click_common
from subprocess import call, PIPE, STDOUT, Popen
from globals import NUM_RACKS, HOSTS_PER_RACK, TIMESTAMP, SCRIPT, EXPERIMENTS

RPYC_PORT = 18861
RPYC_CONNECTIONS = {}

DATA_NET = 1
CONTROL_NET = 2

ADU_PRELOAD = os.path.expanduser('~/sdrt/sdrt-ctrl/lib/sdrt-ctrl.so')

RACKS = []
PHYSICAL_NODES = []
NODES = {}

CURRENT_CC = ''

THREADS = []
THREAD_LOCK = threading.Lock()


##
# Experiment commands
##
def initializeExperiment():
    print '--- starting experiment...'
    print '--- clearing local arp...'
    call([os.path.expanduser('~/sdrt/cloudlab/arp_clear.sh')])
    print '--- done...'

    print '--- populating physical hosts...'
    PHYSICAL_NODES.append('')
    for i in xrange(1, NUM_RACKS+1):
        PHYSICAL_NODES.append('host%d' % i)
    print '--- done...'

    print '--- populating vhosts...'
    RACKS.append([])
    for i in xrange(1, NUM_RACKS+1):
        RACKS.append([])
        NODES['host%d' % i] = node('host%d' % i, PHYSICAL_NODES[i])
        for j in xrange(1, HOSTS_PER_RACK+1):
            RACKS[-1].append(node('h%d%d' % (i, j), PHYSICAL_NODES[i]))
            NODES['h%d%d' % (i, j)] = RACKS[-1][-1]
    print '--- done...'

    print '--- connecting to rpycd...'
    connect_all_rpyc_daemon()
    print '--- done...'

    # print '--- killing any left over pings...'
    # kill_all_ping()
    # print '--- done...'

    # print '--- restarting sockperf...'
    # kill_all_sockperf()
    # launch_all_sockperf()
    # print '--- done...'

    print '--- setting CC to reno...'
    setCC('reno')
    print '--- done...'

    print '--- launching flowgrindd...'
    launch_all_flowgrindd()
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
    # print '--- killing left over sockperf...'
    # kill_all_sockperf()
    # print '--- done...'
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
            RPYC_CONNECTIONS[phost] = rpyc.connect(phost, RPYC_PORT)
        except:
            print 'could not connect to ' + phost
            bad_hosts.append(phost)
    map(lambda x: PHYSICAL_NODES.remove(x), bad_hosts)




##
# ping
##
def rack_ping(src, dst):
    src_node = NODES['host%d' % src]
    dst_node = NODES['host%d' % dst]
    fn = click_common.FN_FORMAT % ('ping-%s-%s' % (src, dst))
    src_node.ping(dst_node, fn)


def all_rack_ping():
    for i in xrange(1, NUM_RACKS+1):
        for j in xrange(1, NUM_RACKS+1):
            if i != j:
                rack_ping(i, j)


def kill_all_ping():
    results = []
    for phost in PHYSICAL_NODES[1:]:
        kp = rpyc.async(RPYC_CONNECTIONS[phost].root.kill_all_ping)
        results.append(kp())
    for r in results:
        while not r.ready:
            time.sleep(0.1)


##
# sockperf
##
def rack_sockperf(src, dst):
    src_node = NODES['host%d' % src]
    dst_node = NODES['host%d' % dst]
    fn = click_common.FN_FORMAT % ('sockperf-%s-%s' % (src, dst))
    src_node.sockperf(dst_node, fn)


def all_rack_sockperf():
    for i in xrange(1, NUM_RACKS+1):
        for j in xrange(1, NUM_RACKS+1):
            if i != j:
                rack_sockperf(i, j)


def launch_all_sockperf():
    results = []
    for phost in PHYSICAL_NODES[1:]:
        kp = rpyc.async(RPYC_CONNECTIONS[phost].root.launch_sockperf_daemon)
        results.append(kp())
    for r in results:
        while not r.ready:
            time.sleep(0.1)


def kill_all_sockperf():
    results = []
    for phost in PHYSICAL_NODES[1:]:
        kp = rpyc.async(RPYC_CONNECTIONS[phost].root.kill_all_sockperf)
        results.append(kp())
    for r in results:
        while not r.ready:
            time.sleep(0.1)


##
# Congestion Control
##
def set_cc_host(phost, cc):
    RPYC_CONNECTIONS[phost].root.set_cc(cc)

def setCC(cc):
    global CURRENT_CC
    ts = []
    for phost in PHYSICAL_NODES[1:]:
        ts.append(threading.Thread(target=set_cc_host,
                                   args=(phost, cc)))
        ts[-1].start()
    map(lambda t: t.join(), ts)
    if CURRENT_CC and cc != CURRENT_CC:
        launch_all_flowgrindd()
    CURRENT_CC = cc

##
# flowgrind
##
def launch_flowgrindd(phost):
    RPYC_CONNECTIONS[phost].root.flowgrindd()


def launch_all_flowgrindd():
    ts = []
    for phost in PHYSICAL_NODES[1:]:
        ts.append(threading.Thread(target=launch_flowgrindd,
                                   args=(phost,)))
        ts[-1].start()
    map(lambda t: t.join(), ts)


def get_flowgrind_host(h):
    # return '10.%s.10.%s/10.%s.10.%s' % (DATA_NET, h[1],
    #                                     CONTROL_NET, h[1])
    return '10.%s.%s.%s/10.%s.%s.%s' % (DATA_NET, h[1], h[2:],
                                        CONTROL_NET, h[1], h[2:])


def flowgrind(settings):
    cmd = 'flowgrind -I '
    flows = []
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
    cmd += '-n %s ' % len(flows)
    for i, f in enumerate(flows):
        if 'time' not in f:
            f['time'] = 2
        cmd += '-F %d -Hs=%s,d=%s -Ts=%d -Ss=8948 ' % (i, get_flowgrind_host(f['src']),
                                                       get_flowgrind_host(f['dst']),
                                                       f['time'])
    cmd = 'echo "%s" && %s' % (cmd, cmd)
    print cmd
    fn = click_common.FN_FORMAT % ('flowgrind')
    print fn
    EXPERIMENTS.append(fn)
    backgroundRun(cmd, fn)
    kill_all_ping()


##
# VHost Node
##
class job:
    def __init__(self, type, server, fn, result, conn, async):
        self.type = type
        self.server = server
        self.fn = fn
        self.result = result
        self.conn = conn
        self.async = async


class node:
    def __init__(self, hostname, parent):
        self.hostname = hostname
        self.work = []
        self.parent = parent

    def ping(self, dst, fn):
        if dst.__class__ == node:
            dst = dst.hostname
        c = rpyc.connect(self.parent, RPYC_PORT)
        pa = rpyc.async(c.root.ping)
        r = pa(dst)
        self.work.append(job('ping', dst, fn, r, c, pa))
        print fn
        EXPERIMENTS.append(fn)

    def sockperf(self, dst, fn):
        if dst.__class__ == node:
            dst = dst.hostname
        c = rpyc.connect(self.parent, RPYC_PORT)
        spa = rpyc.async(c.root.sockperf)
        r = spa(dst)
        self.work.append(job('sockperf', dst, fn, r, c, spa))
        print fn
        EXPERIMENTS.append(fn)

    def get_dones(self):
        dones, not_dones = [], []
        for x in self.work:
            if x.result.ready:
                dones.append(x)
            else:
                not_dones.append(x)
        self.work = not_dones
        return dones

    def save_done(self, done):
        out = done.result.value
        print '%s: %s %s done' % (self.hostname, done.type, done.server)
        print 'fn = %s' % (done.fn)
        open(done.fn, 'w').write(out)


def waitWork(host):
    print 'waiting on %s (%s jobs)' % (host.hostname, len(host.work))
    while host.work:
        ds = host.get_dones()
        if ds:
            for d in ds:
                host.save_done(d)


def waitOnWork():
    hosts = NODES.values()
    map(waitWork, hosts)


##
# Running shell commands
##
def run(cmd, printOutput=True, checkRC=True, redirect=PIPE, input=""):
    def preexec():  # don't forward signals
        os.setpgrp()

    out = ""
    if input:
        p = Popen(cmd, shell=True, stdout=redirect, stderr=STDOUT,
                  stdin=PIPE, preexec_fn=preexec)
        p.stdin.write(input)
    else:
        p = Popen(cmd, shell=True, stdout=redirect, stderr=STDOUT,
                  preexec_fn=preexec)
    while redirect == PIPE:
        line = p.stdout.readline()  # this will block
        if not line:
            break
        if printOutput:
            sys.stdout.write(line)
        out += line
    rc = p.poll()
    while rc is None:
        #time.sleep(1)
        rc = p.poll()
    if checkRC and rc != 0:
        raise Exception("subprocess.CalledProcessError: Command '%s'"
                        "returned non-zero exit status %s\n"
                        "output was: %s" % (cmd, rc, out))
    return (rc, out)


##
# Threading
##
def backgroundRun(cmd, fn):
    try:
        out = run(cmd)[1]
    except Exception, e:
        print e
        out = str(e)
    if fn:
        f = open(fn, 'w')
        f.write(out)
        f.close()


def threadRun(cmd, fn=None):
    THREAD_LOCK.lock()
    THREADS.append(threading.Thread(target=backgroundRun,
                                    args=(cmd, fn)).start())
    THREAD_LOCK.unlock()


def waitOnThreads():
    while THREADS:
        time.sleep(1)
