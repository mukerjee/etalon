#!/usr/bin/python

import os
import argparse

cnt = 0
curPath = 1
PACKET_BW = '10mbit'
CIRCUIT_BW = '100mbit'
OTHER_BW = '1gbit'

HOSTS = ["172.31.20.110", "172.31.20.108"]

def setPath(src, dst, cls):
    os.system("sudo iptables -t mangle -A POSTROUTING -o eth0 -s %s -d %s -j CLASSIFY --set-class 1:%d"
            %(src, dst, cls))

def unsetPath(src, dst, cls):
    os.system("sudo iptables -t mangle -D POSTROUTING -o eth0 -s %s -d %s -j CLASSIFY --set-class 1:%d"
            %(src, dst, cls))

def setIPT():
    global HOSTS
    queue_num = 1
    for s in HOSTS:
        for d in HOSTS:
            if s == d: 
                continue
            os.system("sudo iptables -I FORWARD -s %s -d %s -j NFQUEUE --queue-num %d"
                    %(s,d,queue_num))
            queue_num = queue_num+1
    os.system("sudo iptables -nvL")

def unsetIPT():
    global HOSTS
    queue_num = 1 
    for s in HOSTS:
        for d in HOSTS:
            if s == d:
                continue
            os.system("sudo iptables -D FORWARD -s %s -d %s -j NFQUEUE --queue-num %d"
                    % (s,d,queue_num))
            queue_num = queue_num+1
    os.system("sudo iptables -nvL")

def setTC():
    os.system("tc qdisc add dev eth0 root handle 1: htb default 201")
    os.system("tc class add dev eth0 parent 1: classid 1:201 htb rate %s ceil %s"
                % (OTHER_BW, OTHER_BW))

    os.system("tc class add dev eth0 parent 1: classid 1:%d htb rate %s ceil %s"
                    % (1, PACKET_BW, PACKET_BW))

    os.system("tc class add dev eth0 parent 1: classid 1:%d htb rate %s ceil %s"
                    % (101, CIRCUIT_BW, CIRCUIT_BW))
    
    os.system("tc qdisc show")
    os.system("tc class show dev eth0")

def unsetTC():
    os.system("tc qdisc del dev eth0 root")
    
if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument("--tc", help="unset or set TC")
    parser.add_argument("--ipt", help="unset or set ipt")
    parser.add_argument("--path", help="packet or circuit path")
    args = parser.parse_args()
    if args.tc == "unset":
        unsetTC()
    if args.tc == "set":
        setTC()
    if args.ipt == "unset":
        unsetIPT()
    if args.ipt == "set":
        setIPT()
    if args.path == "packet":
        setPath("172.31.20.110","172.31.20.108",1)
        setPath("172.31.20.108","172.31.20.110",1)
    if args.path == "circuit":
        setPath("172.31.20.110","172.31.20.108",101)
        setPath("172.31.20.108","172.31.20.110",101)
