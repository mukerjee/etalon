import threading
import subprocess
import paramiko
import argparse
from time import sleep
import re

publicIP = {'switch':[], 'hosts':[]}
privateIP = {'switch':[], 'hosts':[]}
ports = []
port_mapping = {}
num_hosts = 0

def iperf_server(host, port):
    cmd = 'sudo nice --20 iperf3 -s -p %d &' % (port)
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host, username='ec2-user')
    stdin, stdout, stderr = ssh.exec_command(cmd)

def iperf_kill_server(host):
    cmd = 'sudo killall iperf3'
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host, username='ec2-user')
    stdin, stdout, stderr = ssh.exec_command(cmd)

def iperf_client(host, remote, port, time):
    cmd = 'sudo nice --20 iperf3 -c %s -p %d -t %d' % (remote, port, time)
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host, username='ec2-user')
    stdin, stdout, stderr = ssh.exec_command(cmd)

    output = stdout.readlines()
    print output[len(output)-3]

def run_exp(time, exp):
    global num_hosts
    threads = []
    if exp == 'all':
        for i in range(0, num_hosts):
            for j in range(0, num_hosts):
                if i == j:
                    continue
                iperf_server(publicIP['hosts'][i], ports[j])
                if i not in port_mapping.keys():
                    port_mapping[i] = {}
                port_mapping[i][j] = ports[j]
        sleep(1)

        for i in range(0, num_hosts):
            for j in range(0, num_hosts):
                if i == j:
                    continue
                t = threading.Thread(target=iperf_client, args=(publicIP['hosts'][i], privateIP['hosts'][j], port_mapping[j][i], time, ))
                t.start()
                threads.append(t)
        for t in threads:
            t.join()
    else:
        for i in range(0, num_hosts, 2):
            iperf_server(publicIP['hosts'][i], ports[0])
            if i not in port_mapping.keys():
                port_mapping[i] = {}
            port_mapping[i][i+1] = ports[0]
        sleep(1)

        for i in range(1, num_hosts, 2):
            print privateIP['hosts'][i-1]
            t = threading.Thread(target=iperf_client, args=(publicIP['hosts'][i], privateIP['hosts'][i-1], port_mapping[i-1][i], time, ))
            t.start()
            threads.append(t)

        print 'Waiting...'
        for t in threads:
            t.join()


    for i in range(0, num_hosts):
        iperf_kill_server(publicIP['hosts'][i])

def init():
    global num_hosts, ports
    base_port = 5201

    switch_idx = -1

    cmd = ("aws ec2 describe-instances "
           "--query \"Reservations[*].Instances[*].PublicIpAddress\" "
           "--output=text")

    print cmd
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

    for i in range(0, num_hosts):
        ports.append(base_port+i)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--exp", help="experiment type (all or single)", default="single")
    parser.add_argument("--time", help="iperf running time", default="10")
    args = parser.parse_args()
    init()
    run_exp(int(args.time), args.exp)


