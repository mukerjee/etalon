#!/usr/bin/env python

import socket
import time
from common import run

n = 3
PACKET_BW = '10mbit'
CIRCUIT_BW = '100mbit'
OTHER_BW = '1gbit'

lines = open('handles.ec2').read().split('\n')[:-1]
lines = [l.split('#')[1] for l in lines if l.split('#')[0].strip() != 'switch']

IPs = [socket.gethostbyname(h) for h in lines]

# n = 64
# IPs = ['192.168.1.%s' % str(i+1) for i in xrange(n)]

run("tc qdisc del dev eth0 root", printOutput=False, checkRC=False)

run("tc qdisc add dev eth0 root handle 1: htb default %s" % str(2*n+1))

# packet
for i in xrange(1, n+1):
    run("tc class add dev eth0 parent 1: classid 1:%s htb rate %s ceil %s"
        % (str(i), PACKET_BW, PACKET_BW))

# circuit
for i in xrange(n+1, 2*n+1):
    run("tc class add dev eth0 parent 1: classid 1:%s htb rate %s ceil %s"
        % (str(i), CIRCUIT_BW, CIRCUIT_BW))

# extra class for ssh
run("tc class add dev eth0 parent 1: classid 1:%s htb rate %s ceil %s"
    % (str(2*n+1), OTHER_BW, OTHER_BW))

# packet
for i in xrange(1, n+1):
    run("tc filter add dev eth0 protocol ip parent 1:0 prio 2 u32 match "
        "ip dst %s flowid 1:%s" % (IPs[i-1], str(i)))

# circuit
for i in xrange(n+1, 2*n+1):
    run("tc filter add dev eth0 handle 801::%s protocol ip parent 1:0 "
        "prio 1 u32 match ip src %s match ip dst %s flowid 1:%s"
        % (str(100+i-n), IPs[(i % n)], IPs[(i % n)-1], str(i)))


# start = time.time()
# for t in xrange(1000):
#     fp = open('tc_batch.txt', 'w')
    
#     for i in xrange(n+1, 2*n+1):
#         fp.write("filter del dev eth0 parent 1: handle 801::%s prio 1 " \
#                  "protocol ip u32\n" % (str(100+i-n)))

#     for i in xrange(n+1, 2*n+1):
#         fp.write("filter add dev eth0 handle 801::%s protocol ip parent 1:0 " \
#                  "prio 1 u32 match ip src %s match ip dst %s flowid 1:%s\n" \
#                  % (str(100+i-n), IPs[(i % n)], IPs[(i % n)-1], str(i)))

#     fp.close()

#     run("tc -b tc_batch.txt")

# print time.time() - start
