#!/usr/bin/python

import subprocess
import copy
import time
import threading
import multiprocessing
import sys
import os
import socket
import select
import paramiko
from subprocess import Popen, PIPE, STDOUT

RPC_PORT = 43278
CHECK_TIMEOUT = 15

NODES_FILE = os.path.expanduser('~/sdrt/cloudlab/common/handles.cloudlab')
MININET_NODES_FILE = os.path.expanduser('~/sdrt/cloudlab/common/mininet.cloudlab')

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
THREADLOCK = threading.Lock()


##
## Experiment commands
##
def initializeExperiment():
    global NUM_RACKS, HOSTS_PER_RACK, PHYSICAL_NODES, RACKS  ##### remove last two ####

    print '--- starting experiment...'
    print '--- clearing local arp...'
    subprocess.call([os.path.expanduser('~/sdrt/cloudlab/clear_arp.sh')])
    print '--- done...'

    print '--- parsing host handles...'
    f = open(NODES_FILE).read().split('\n')[:-1]
    for line in f[1:]:
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
    print '--- done...'

    ######## Remove this after testing #######
    PHYSICAL_NODES = PHYSICAL_NODES[:2]
    RACKS = RACKS[:3]
    print PHYSICAL_NODES, RACKS
    #######                           #######

    print '--- checking if mininet is running on hosts...'
    p = multiprocessing.Pool()
    p.map(checkVHost, [h for r in RACKS for h in r])
    p.close()
    print '--- done...'

    initializeClickControl()

    print '--- setting default click buffer sizes and traffic sources...'
    setQueueSize(10)
    setEstimateTrafficSource('QUEUE')
    print '--- done...'
    print '--- done starting experiment...'
    print
    print

def checkVHost(handle):
    i = 0
    while 1:
        try:
            print 'checking if %s is up yet... (%d)' % (handle, i)
            sshRun(handle, 'uname -r', printOutput=False)
            break
        except:
            i += 1

##
## Running click commands
##    
def initializeClickControl():
    global CLICK_SOCKET
    print '--- connecting to click socket...'
    CLICK_SOCKET = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    CLICK_SOCKET.connect((TCP_IP, TCP_PORT))
    CLICK_SOCKET.recv(BUFFER_SIZE)
    print '--- done...'

def clickWriteHandler(element, handler, value):
    message = "WRITE %s.%s %s\n" % (element, handler, value)
    print message.strip()
    CLICK_SOCKET.send(message)
    print CLICK_SOCKET.recv(BUFFER_SIZE).strip()

def clickReadHandler(element, handler):
    message = "READ %s.%s\n" % (element, handler)
    print message.strip()
    CLICK_SOCKET.send(message)
    data = CLICK_SOCKET.recv(BUFFER_SIZE).strip()
    print data
    return data

def setQueueSize(size):
    for i in xrange(len(RACKS)):
        for j in xrange(len(RACKS)):
            clickWriteHandler('hybrid_switch/q%d%d' % (i, j), 'capacity', size)

def setEstimateTrafficSource(source):
    clickWriteHandler('traffic_matrix', 'setSource', source)

def setQueueResize(b):
    if b:
        clickWriteHandler('runner', 'setDoResize', 'true')
    else:
        clickWriteHandler('runner', 'setDoResize', 'false')


##
## Running shell commands
##
def sshRun(hostname, cmd, printOutput=True):
    out = ""

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(hostname, timeout=1)
    sesh = client.get_transport().open_session()
    sesh.get_pty()
    sesh.exec_command(cmd)

    while True:
        if sesh.exit_status_ready():
            break
        rs, _, _ = select.select([sesh], [], [], 0.0)
        if len(rs) > 0:
            new = sesh.recv(1024)
            out += new
            if printOutput:
                sys.stdout.write(new)
                sys.stdout.flush()

    client.close()

    return (out, sesh.recv_exit_status())


##
## Threading
##
def threadRun(hostname, handle, cmd, current, total, fn, po):
    try:
        out = sshRun(hostname, cmd, printOutput=po)[0]
    except Exception, e:
        print e
        out = ''
    THREADLOCK.acquire()
    THREADS.remove(hostname)
    if total:
        out = ('(%s/%s) %s:\n' % (current, total, handle)) + out
        if po:
            print '(%s/%s) %s: done\n' % (current, total, handle)
    THREADLOCK.release()
    if fn:
        open(fn, 'w').write(out)

def runOnNode(handle, cmd, current=0, total=0, preload=True, printOutput=True, fn=None):
    try:
        hostname = handle.strip()
        print hostname, cmd, fn
        while len(THREADS) >= MAX_CONCURRENT:
            time.sleep(1)
        THREADLOCK.acquire()
        if preload:
            cmd = 'LD_PRELOAD=%s %s' % (ADU_PRELOAD, cmd)
        threading.Thread(target=threadRun,
                         args=(hostname, handle, cmd, current, total, 
                               background, fn, printOutput)).start()
        THREADS.append(hostname)
        THREADLOCK.release()
    except Exception, e:
        print e

def waitOnNodes():
    while THREADS:
        time.sleep(1)
