import threading
import subprocess
import paramiko
import argparse

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

    print stdout.readlines()[len(stdout.readlines())-3]

def run_exp(time):
    global num_hosts
    threads = []
    for i in range(0, num_hosts):
        for j in range(0, num_hosts):
            if i == j:
                continue
            iperf_server(publicIP['hosts'][i], ports[j])
            if i not in port_mapping.keys():
                port_mapping[i] = {}
            port_mapping[i][j] = ports[j]

    for i in range(0, num_hosts):
        for j in range(0, num_hosts):
            if i == j:
                continue
            t = threading.Thread(target=iperf_client, args=(publicIP['hosts'][i], privateIP['hosts'][j], port_mapping[j][i], time, ))
            t.start()
            threads.append(t)
    for t in threads:
        t.join()

    for i in range(0, num_hosts):
        iperf_kill_server(publicIP['hosts'][i])

def init():
    global num_hosts, ports
    base_port = 5201

    cmd = ("aws ec2 describe-instances "
           "--query \"Reservations[*].Instances[*].PublicIpAddress\" "
           "--output=text")
    output = subprocess.check_output(cmd, shell=True)
    publicIP['switch'] = [output.split("\n")[1]]
    publicIP['hosts'] = output.split("\n")[0].split("\t")

    cmd = ("aws ec2 describe-instances "
           "--query \"Reservations[*].Instances[*].PrivateIpAddress\" "
           "--output=text")
    output = subprocess.check_output(cmd, shell=True)
    privateIP['switch'] = [output.split("\n")[1]]
    privateIP['hosts'] = output.split("\n")[0].split("\t")
    num_hosts = len(publicIP['hosts'])
    print num_hosts

    for i in range(0, num_hosts):
        ports.append(base_port+i)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--time", help="iperf running time", default="10")
    args = parser.parse_args()
    init()
    run_exp(int(args.time))


