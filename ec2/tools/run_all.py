#!/usr/bin/python

import threading
import time

import sys
sys.path.append('../common/')

from common import sshRun, run, NODES_FILE


# run some command (see below) on all instances

commands = {}
commands['uname'] = '"uname -r"'
commands['ls'] = '"sudo ls"'
commands['update'] = ('rsync --rsh=\'ssh -o StrictHostKeyChecking=no -p %s\' '
                      '-ar --files-from=../common/bin-files ../ '
                      '%s@%s:~/dc/')

current = 1
MAX_CONCURRENT = 10

threads = []
threadlock = None


def threadRun(hostname, handle, cmd, total):
    global current
    try:
        if sys.argv[1] == 'update':
            out = run(cmd % hostname, False)[0]
        else:
            out = sshRun(hostname, cmd, False)[0]
    except Exception, e:
        print e
        out = ''
    threadlock.acquire()
    threads.remove(hostname)
    out = ('(%s/%s) %s: ' % (current, total, handle)) + out
    current += 1
    threadlock.release()
    sys.stdout.write(out)
    sys.stdout.flush()


def main():
    global threads, threadlock
    threads = []
    threadlock = threading.Lock()
    if len(sys.argv) < 2:
        print 'usage %s uname | ... [nodes_file]' % (sys.argv[0])
        sys.exit(-1)

    nodesFile = NODES_FILE if len(sys.argv) < 3 else sys.argv[2]
    lines = open(nodesFile, 'r').read().split('\n')[:-1]
    nodes = [line.split('#') for line in lines]

    total = len(nodes)
    cmd = commands[sys.argv[1]]
    print 'launching %s' % (cmd)
    for handle, hostname in nodes:
        try:
            hostname = hostname.strip()
            while len(threads) >= MAX_CONCURRENT:
                time.sleep(1)
            threadlock.acquire()
            threading.Thread(target=threadRun,
                             args=(hostname, handle, cmd, total)).start()
            threads.append(hostname)
            threadlock.release()
        except Exception, e:
            print e
    while threads:
        time.sleep(1)

if __name__ == '__main__':
    main()
