#!/usr/bin/python

import time
import threading
import sys
import os
import tarfile
import rpyc
import click_common
from subprocess import call, PIPE, STDOUT, Popen
from globals import NUM_RACKS, HOSTS_PER_RACK, TIMESTAMP, SCRIPT, \
    EXPERIMENTS, PHYSICAL_NODES, RPYC_CONNECTIONS, RPYC_PORT

DATA_NET = 1
CONTROL_NET = 2

ADU_PRELOAD = os.path.expanduser('~/sdrt/sdrt-ctrl/lib/sdrt-ctrl.so')

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

    print '--- connecting to rpycd...'
    connect_all_rpyc_daemon()
    print '--- done...'

    print '--- setting CC to reno...'
    click_common.setCC('reno')
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
        cmd += '-F %d -Hs=%s,d=%s -Ts=%d -Ss=8948 ' % \
            (i, get_flowgrind_host(f['src']), get_flowgrind_host(f['dst']),
             f['time'])
    cmd = 'echo "%s" && %s' % (cmd, cmd)
    print cmd
    fn = click_common.FN_FORMAT % ('flowgrind')
    print fn
    runWriteFile(cmd, fn)


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
        sys.stdout.write(line)
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
