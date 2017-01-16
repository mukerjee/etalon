#!/usr/bin/python

import os
import argparse

cnt = 0
curPath = 1
PACKET_BW = '10mbit'
CIRCUIT_BW = '100mbit'
OTHER_BW = '1gbit'

HOSTS = []
circuit_mapping = {}
packet_mapping = {}

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
            os.system("sudo iptables -I FORWARD -s %s -d %s -j NFQUEUE --queue-num %d --queue-bypass"
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
            os.system("sudo iptables -D FORWARD -s %s -d %s -j NFQUEUE --queue-num %d --queue-bypass"
                    % (s,d,queue_num))
            queue_num = queue_num+1
    os.system("sudo iptables -nvL")

def setTC():
    global circuit_mapping, packet_mapping

    os.system("tc qdisc add dev eth0 root handle 1: htb default 99")
    os.system("tc class add dev eth0 parent 1: classid 1:99 htb rate %s ceil %s"
                % (OTHER_BW, OTHER_BW))

    for i in range(0, len(HOSTS)):
        os.system("tc class add dev eth0 parent 1: classid 1:%d htb rate %s ceil %s"
                        % (packet_mapping[i], PACKET_BW, PACKET_BW))

    for i in range(0, len(HOSTS)):
        for j in range(0, len(HOSTS)):
            if i == j:
                continue
            os.system("tc class add dev eth0 parent 1: classid 1:%d htb rate %s ceil %s"
                % (circuit_mapping[i][j], CIRCUIT_BW, CIRCUIT_BW))

    os.system("tc qdisc show")
    os.system("tc class show dev eth0")

def unsetTC():
    os.system("tc qdisc del dev eth0 root")

def setPacket():
    global packet_mapping
    print packet_mapping

    for i in range(0, len(HOSTS)):
        for j in range(0, len(HOSTS)):
            if i == j:
                continue
            setPath(HOSTS[i], HOSTS[j], packet_mapping[j])

def setCircuit():
    global circuit_mapping
    for i in range(0, len(HOSTS)):
        for j in range(0, len(HOSTS)):
            if i == j:
                continue
            setPath(HOSTS[i], HOSTS[j], circuit_mapping[i][j])


def init():
    global circuit_mapping, packet_mapping

    fin = open('/home/ec2-user/hosts.txt','r')
    for ip in fin:
        HOSTS.append(ip.rstrip('\r\n'))

    fin.close()
    for i in range(0, len(HOSTS)):
        packet_mapping[i] = i
    cls = 100

    for i in range(0, len(HOSTS)):
        for j in range(0, len(HOSTS)):
            if i == j:
                continue
            if i not in circuit_mapping.keys():
                circuit_mapping[i] = {}
            circuit_mapping[i][j] = cls
            cls = cls + 1


if __name__ == '__main__':
    init()
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
        setPacket()
    if args.path == "circuit":
        setCircuit()
