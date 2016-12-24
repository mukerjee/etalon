#!/usr/bin/env python

import socket
from common import run

# MAX 99 nodes!!

PACKET_BW = '10mbit'
CIRCUIT_BW = '100mbit'
OTHER_BW = '1gbit'

lines = open('handles.ec2').read().split('\n')[:-1]
lines = [l.split('#')[1] for l in lines if l.split('#')[0].strip() != 'switch']
n = len(lines)

IPs = [socket.gethostbyname(h) for h in lines]

run("tc qdisc del dev eth0 root", printOutput=False, checkRC=False)

run("tc qdisc add dev eth0 root handle 1: htb default 201")

# packet
for i in xrange(1, 1+n):
    run("tc class add dev eth0 parent 1: classid 1:%d htb rate %s ceil %s"
        % (i, PACKET_BW, PACKET_BW))

# circuit
for i in xrange(101, 101+n):
    run("tc class add dev eth0 parent 1: classid 1:%d htb rate %s ceil %s"
        % (i, CIRCUIT_BW, CIRCUIT_BW))

# extra class for ssh
run("tc class add dev eth0 parent 1: classid 1:201 htb rate %s ceil %s"
    % (OTHER_BW, OTHER_BW))

# packet
for i in xrange(1, 1+n):
    run("tc filter add dev eth0 protocol ip parent 1:0 prio 1 handle %d fw "
        "flowid 1:%d" % (i, i))

# circuit
for i in xrange(101, 101+n):
    run("tc filter add dev eth0 protocol ip parent 1:0 prio 1 handle %d fw "
        "flowid 1:%d" % (i, i))
