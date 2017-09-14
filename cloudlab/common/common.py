#!/usr/bin/python

import copy
import time
import threading
import sys
import os
import rpyc
from subprocess import Popen, PIPE, STDOUT

RPC_PORT = 43278
CHECK_TIMEOUT = 15

NODES_FILE = os.path.expanduser('~/sdrt/cloudlab/common/handles.cloudlab')
MININET_NODES_FILE = os.path.expanduser('~/sdrt/cloudlab/common/mininet.cloudlab')

SSH_CMD = 'ssh -o StrictHostKeyChecking=no -o ConnectTimeout=1 %s %s'

MY_HANDLE = None
MY_IP = None

CONNECTION_LOOKUP = {}

MININET_SCRIPT = os.path.expanduser('~/sdrt/cloudlab/mininet_startup.py -n')
KILL_SCRIPT = os.path.expanduser('~/sdrt/cloudlab/kill.sh')
ADU_PRELOAD = os.path.expanduser('~/sdrt/sdrt-ctrl/lib/sdrt-ctrl.so')

RACKS = []
PHYSICAL_NODES = []
HANDLE_TO_IP = {}
NUM_RACKS = 0
HOSTS_PER_RACK = 0

TCP_IP = 'localhost'
TCP_PORT = 1239
BUFFER_SIZE = 1024
CLICK_SOCKET = None

MAX_CONCURRENT = 64

THREADS = []
THREADLOCK = None

def initializeExperiment():
    initializeRacks()
    initializeClickControl()
    initializeThreads()

def initializeRacks():
    global NUM_RACKS, HOSTS_PER_RACK
    f = open(NODES_FILE).read().split('\n')[:-1]
    for line in f:
        handle, hostname = [x.strip() for x in line.split('#')]
        PHYSICAL_NODES.append(handle)
        HANDLE_TO_IP[handle] = hostname

    f = open(MININET_NODES_FILE).read().split('\n')[:-1]
    NUM_RACKS = int(f[-1].split('#')[0].strip()[-1])
    RACKS.append([])
    for i in xrange(NUM_RACKS):
        RACKS.append([])
    for line in f:
        handle, ip = [x.strip() for x in line.split('#')]
        RACKS[int(handle[-2])].append(handle)
        HANDLE_TO_IP[handle] = ip
    HOSTS_PER_RACK = len(RACKS[1])

    for pn in PHYSICAL_NODES:
        runOnNode(pn, MININET_SCRIPT, preload=False)
    waitOnNodes()

def stopExperiment():
    for pn in PHYSICAL_NODES:
        runOnNode(pn, KILL_SCRIPT, preload=False)
    waitOnNodes()

def initializeThreads():
    global THREADS, THREADLOCK
    THREADLOCK = threading.Lock()

def initializeClickControl():
    global CLICK_SOCKET
    CLICK_SOCKET = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    CLICK_SOCKET.connect((TCP_IP, TCP_PORT))
    CLICK_SOCKET.recv(BUFFER_SIZE)

def clickWriteHandler(element, handler, value):
    message = "WRITE %s.%s %s\n" % (element, handler, value)
    CLICK_SOCKET.send(message)
    data = CLICK_SOCKET.recv(BUFFER_SIZE)

def clickReadHandler(element, handler):
    message = "READ %s.%s\n" % (element, handler)
    CLICK_SOCKET.send(message)
    return CLICK_SOCKET.recv(BUFFER_SIZE)

def setQueueSize(size):
    for i in len(RACKS):
        for j in len(RACKS):
            clickWriteHandler('hybrid_switch/q%d%d', 'capacity', size)

def setEstimateTrafficSource(source):
    clickWriteHandler('traffic_matrix', 'setSource', source)

def setQueueResize(b):
    if b:
        clickWriteHandler('runner', 'setDoResize', 'true')
    else:
        clickWriteHandler('runner', 'setDoResize', 'false')


##
## Misc
##
def printTime(s):
    print '%s: %s' % (time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime()), s)


##
## Running shell commands
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
    return (out, rc)


def sshRun(hostname, cmd, *args):
    return run(SSH_CMD % (hostname, cmd), *args)


##
## Node info manipulation
##
def my_node_info():
    global MY_HANDLE, MY_IP
    MY_IP = run("ifconfig eth3 | grep 10.10.2 | xargs | "
                "cut -f2 -d' ' | cut -f2 -d':'", False)[0]
    if MY_HANDLE:
        return (MY_HANDLE, MY_IP)
    f = open(MININET_NODES_FILE).read().split('\n')[:-1]
    for line in f:
        l = [x.strip() for x in line.split('#')]
        if MY_IP == l[1]:
            MY_HANDLE = l[0]
    return (MY_HANDLE, MY_IP)

##
## RPCs
##
def rpcClose():
    for c in CONNECTION_LOOKUP.values():
        c.close()


def rpc(dest, cmd, args, silent=False):
    c = None
    try:
        if dest in CONNECTION_LOOKUP:
            c = CONNECTION_LOOKUP[dest]
        else:
            c = rpyc.connect(dest, RPC_PORT, keepalive=True,
                             config={'allow_pickle': True})
            # CONNECTION_LOOKUP[dest] = c  # put this back in if we save conn
        if not silent:
            print 'Sending RPC: (%s, %s, %s)' % (dest, cmd, args)
        my_node_info()
        s = 'c.root.%s(MY_HANDLE, *args)' % cmd
        out = copy.deepcopy(eval(s))
        c.close()  # take this out if we save connections!!
        return out
    except Exception, e:
        if c:
            c.close()  # take this out if we save connections!!
        if silent:
            return
        else:
            raise e


def asyncRPC(dest, cmd, arguments, silent=False):
    threading.Thread(target=rpc, args=(dest, cmd, arguments, silent)).start()


##
## Threading
##
def stopAndJoin(threads):
    for t in threads:
        t.stop()
    for t in threads:
        t.join()


class ThreadStop(threading.Thread):
    def __init__(self):
        super(ThreadStop, self).__init__()
        self.runningFlag = threading.Event()
        self.runningFlag.set()

    def stop(self):
        self.runningFlag.clear()

def threadRun(hostname, handle, cmd, current, total):
    try:
        out = sshRun(hostname, cmd, False)[0]
    except Exception, e:
        print e
        out = ''
    threadlock.acquire()
    threads.remove(hostname)
    if total:
        out = ('(%s/%s) %s: ' % (current, total, handle)) + out
        sys.stdout.write(out)
        sys.stdout.flush()
    threadlock.release()

def runOnNode(handle, cmd, current=0, total=0, preload=True):
    try:
        hostname = handle.strip()
        while len(threads) >= MAX_CONCURRENT:
            time.sleep(1)
        THREADLOCK.acquire()
        if preload:
            cmd = 'LD_PRELOAD=%s %s' (ADU_PRELOAD, cmd)
        threading.Thread(target=threadRun,
                         args=(hostname, handle, cmd, current, total)).start()
        THREADS.append(hostname)
        THREADLOCK.release()
    except Exception, e:
        print e

def waitOnNodes():
    while THREADS:
        sleep(1)
