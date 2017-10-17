#!/usr/bin/python

import time
import threading
import sys
import os
import rpyc
import tarfile
from subprocess import call, PIPE, STDOUT, Popen
from click_common import initializeClickControl, setConfig

RPYC_PORT = 18861

ADU_PRELOAD = os.path.expanduser('~/sdrt/sdrt-ctrl/lib/sdrt-ctrl.so')

RACKS = []
PHYSICAL_NODES = []
NUM_RACKS = 8
HOSTS_PER_RACK = 8

THREADS = []
THREAD_LOCK = threading.lock()

TIMESTAMP = int(time.time())
SCRIPT = os.path.splitext(os.path.basename(sys.argv[0]))[0]
EXPERIMENTS = []


##
# Experiment commands
##
def initializeExperiment():
    global NUM_RACKS, HOSTS_PER_RACK

    print '--- starting experiment...'
    print '--- clearing local arp...'
    call([os.path.expanduser('~/sdrt/cloudlab/clear_arp.sh')])
    print '--- done...'

    print '--- populating physical hosts...'
    PHYSICAL_NODES.append('')
    for i in xrange(NUM_RACKS):
        PHYSICAL_NODES.append('host%d' % i)
    print '--- done...'

    print '--- populating vhosts...'
    RACKS.append([])
    for i in xrange(NUM_RACKS):
        RACKS.append([])
        for j in xrange(HOSTS_PER_RACK):
            RACKS[-1].append(node('host%d%d' % (i, j), PHYSICAL_NODES[i]))
    print '--- done...'

    initializeClickControl()

    print '--- setting default click buffer sizes and traffic sources...'
    setConfig({})
    print '--- done...'
    print '--- done starting experiment...'
    print
    print


def stopExperiment():
    tar = tarfile.open("%s-%s.tar.gz" % (TIMESTAMP, SCRIPT), "w:gz")
    for e in EXPERIMENTS:
        tar.add(e)
    tar.close()

    for e in EXPERIMENTS:
        os.remove(e)


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
        self.rpc_conn = rpyc.connect(parent, RPYC_PORT)
        self.iperf_async = rpyc.async(self.rpc_conn.root.iperf_client)

    def iperf(self, server, fn):
        if server.__class__ == node:
            server = server.hostname
        r = self.iperf_async(self.hostname, server)
        self.work.append(job('iperf', server, fn, time, r))

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
