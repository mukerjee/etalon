#!/usr/bin/python

import time
import threading
import sys
import os
import rpyc
import tarfile
from subprocess import call, PIPE, STDOUT, Popen
from globals import NUM_RACKS, HOSTS_PER_RACK, TIMESTAMP, SCRIPT
from click_common import initializeClickControl, setConfig, FN_FORMAT

RPYC_PORT = 18861
RPYC_CONNECTIONS = {}

DATA_NET = 1
CONTROL_NET = 2

ADU_PRELOAD = os.path.expanduser('~/sdrt/sdrt-ctrl/lib/sdrt-ctrl.so')

RACKS = []
PHYSICAL_NODES = []

THREADS = []
THREAD_LOCK = threading.Lock()

EXPERIMENTS = []


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
        for j in xrange(1, HOSTS_PER_RACK+1):
            RACKS[-1].append(node('h%d%d' % (i, j), PHYSICAL_NODES[i]))
    print '--- done...'

    print '--- connecting to rpycd...'
    connect_all_rpyc_daemon()
    print '--- done...'

    print '--- launching flowgrindd...'
    launch_all_flowgrindd()
    print '--- done...'

    initializeClickControl()

    print '--- setting default click buffer sizes and traffic sources...'
    setConfig({})
    print '--- done...'
    print '--- done starting experiment...'
    print
    print


def tarExperiment():
    tar = tarfile.open("%s-%s.tar.gz" % (TIMESTAMP, SCRIPT), "w:gz")
    for e in EXPERIMENTS:
        tar.add(e)
    tar.close()

    for e in EXPERIMENTS:
        os.remove(e)


##
# rpyc_daemon
##
def connect_all_rpyc_daemon():
    for phost in PHYSICAL_NODES[1:]:
        RPYC_CONNECTIONS[phost] = rpyc.connect(phost, RPYC_PORT)


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
    return '10.10.%s.%s/10.10.%s.%s' % (DATA_NET, h, CONTROL_NET, h)


def flowgrind(settings):
    cmd = 'flowgrind -I '
    flows = []
    for f in settings['flows']:
        if f['src'][0] == 'r' and f['dst'][0] == 'r':
            s = int(f['src'][1])
            d = int(f['src'][1])
            for i in xrange(HOSTS_PER_RACK):
                fl = dict(f)
                fl['src'] = 'h%d%d' % (s, i)
                fl['dst'] = 'h%d%d' % (d, i)
                flows.append(fl)
        else:
            flows.append(f)
    cmd += '-N %s ' % len(flows)
    for i, f in enumerate(flows):
        if 'time' not in f:
            f['time'] = 2
        cmd += '-F %d -Hs=%s,d=%s -Ts=%d' % (i, get_flowgrind_host(f['src']),
                                             get_flowgrind_host(f['dst']),
                                             f['time'])
    print cmd
    fn = FN_FORMAT % ('flowgrind')
    EXPERIMENTS.append(fn)
    backgroundRun(cmd, fn)


##
# VHost Node
##
class job:
    def __init__(self, type, server, fn, time, result):
        self.type = type
        self.server = server
        self.fn = fn
        self.result = result


class node:
    def __init__(self, hostname, parent):
        self.hostname = hostname
        self.work = []
    #     self.rpc_conn = rpyc.connect(parent, RPYC_PORT)
    #     self.iperf_async = rpyc.async(self.rpc_conn.root.iperf_client)

    # def iperf(self, server, fn):
    #     if server.__class__ == node:
    #         server = server.hostname
    #     r = self.iperf_async(self.hostname, server)
    #     self.work.append(job('iperf', server, fn, time, r))

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
        rc, sout, serr = done.result.value
        if rc == 0:
            print '%s: %s %s done' % (self.hostname, done.type, done.server)
            print 'fn = %s' % (done.fn)
            open(done.fn, 'w').write(sout)
        else:
            print '%s: error in %s %s %s' % (self.hostname, done.type,
                                             done.server, done.time)
            print 'fn = %s' % (done.fn)
            sys.stdout.write(sout)
            sys.stdout.write(serr)


def waitWork(host):
    print 'waiting on %s (%s jobs)' % (host.hostname, len(host.work))
    while host.work:
        ds = host.get_dones()
        if ds:
            for d in ds:
                host.save_done(d)


def waitOnWork():
    hosts = [host for rack in RACKS for host in rack]
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
