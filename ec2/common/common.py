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

NODES_FILE = '../common/handles.ec2'

SSH_CMD = 'ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 ec2-user@%s %s'

MY_HANDLE = None
MY_DNS_NAME = None

CONNECTION_LOOKUP = {}


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
    global MY_HANDLE, MY_DNS_NAME
    MY_DNS_NAME = run('curl -m 1 -s '
                      'http://169.254.169.254/'
                      'latest/meta-data/public-hostname', printOutput=False)[0]
    if MY_HANDLE:
        return (MY_HANDLE, MY_DNS_NAME)
    f = open(NODES_FILE).read().split('\n')[:-1]
    for line in f:
        l = [x.strip() for x in line.split('#')]
        if MY_DNS_NAME == l[1]:
            MY_HANDLE = l[0]
    return (MY_HANDLE, MY_DNS_NAME)

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

