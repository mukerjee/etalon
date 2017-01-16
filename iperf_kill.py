#!/usr/bin/python

import threading
import subprocess
import paramiko
import argparse

import re

publicIP = {'switch':[], 'hosts':[]}
privateIP = {'switch':[], 'hosts':[]}
ports = []
port_mapping = {}
num_hosts = 0

throughput = []
threadLock = threading.Lock()

def iperf_kill_server(host):
    cmd = 'sudo killall iperf3'
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host, username='ec2-user')
    stdin, stdout, stderr = ssh.exec_command(cmd)


def run_exp(time, exp):
    for i in range(0, num_hosts):
        iperf_kill_server(publicIP['hosts'][i])


def init():
    global num_hosts, ports

    switch_idx = -1

    cmd = ("aws ec2 describe-instances "
           "--query \"Reservations[*].Instances[*].PublicIpAddress\" "
           "--output=text")

    output = subprocess.check_output(cmd, shell=True)
    ip_list = re.split('\r|\t|\n', output)
    for i in range(0, len(ip_list)):
        if ip_list[i] == '':
            continue
        if ip_list[i].startswith('3'):
            publicIP['switch'] = [ip_list[i]]
            switch_idx = i
        else:
            publicIP['hosts'].append(ip_list[i])

    cmd = ("aws ec2 describe-instances "
           "--query \"Reservations[*].Instances[*].PrivateIpAddress\" "
           "--output=text")
    output = subprocess.check_output(cmd, shell=True)
    ip_list = re.split('\r|\t|\n', output)
    for i in range(0, len(ip_list)):
        if ip_list[i] == '':
            continue
        if i == switch_idx:
            privateIP['switch'] = [ip_list[i]]
        else:
            privateIP['hosts'].append(ip_list[i])

    num_hosts = len(publicIP['hosts'])
    print 'Number of running hosts: %d' % (num_hosts)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--exp", help="experiment type (all or single)", default="single")
    parser.add_argument("--time", help="iperf running time", default="10")
    args = parser.parse_args()
    init()
    run_exp(int(args.time), args.exp)


