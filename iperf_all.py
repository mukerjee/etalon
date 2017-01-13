#!/usr/bin/python

import threading
import subprocess
import paramiko
import argparse
from time import sleep
from mininet.net import Mininet
import select

import re

publicIP = {'switch':[], 'hosts':[]}
privateIP = {'switch':[], 'hosts':[]}
ports = []
port_mapping = {}
num_hosts = 0

throughput = []
threadLock = threading.Lock()

def myexec(ssh, cmd, timeout=100000, want_exitcode=False):
  # one channel per command
  stdin, stdout, stderr = ssh.exec_command(cmd)
  # get the shared channel for stdout/stderr/stdin
  channel = stdout.channel

  # we do not need stdin.
  stdin.close()
  # indicate that we're not going to write to that channel anymore
  channel.shutdown_write()

  # read stdout/stderr in order to prevent read block hangs
  stdout_chunks = []
  stdout_chunks.append(stdout.channel.recv(len(stdout.channel.in_buffer)))
  # chunked read to prevent stalls
  while not channel.closed or channel.recv_ready() or channel.recv_stderr_ready():
      # stop if channel was closed prematurely, and there is no data in the buffers.
      got_chunk = False
      readq, _, _ = select.select([stdout.channel], [], [], timeout)
      for c in readq:
          if c.recv_ready():
              stdout_chunks.append(stdout.channel.recv(len(c.in_buffer)))
              got_chunk = True
          if c.recv_stderr_ready():
              # make sure to read stderr to prevent stall
              stderr.channel.recv_stderr(len(c.in_stderr_buffer))
              got_chunk = True
      '''
      1) make sure that there are at least 2 cycles with no data in the input buffers in order to not exit too early (i.e. cat on a >200k file).
      2) if no data arrived in the last loop, check if we already received the exit code
      3) check if input buffers are empty
      4) exit the loop
      '''
      if not got_chunk \
          and stdout.channel.exit_status_ready() \
          and not stderr.channel.recv_stderr_ready() \
          and not stdout.channel.recv_ready():
          # indicate that we're not going to read from this channel anymore
          stdout.channel.shutdown_read()
          # close the channel
          stdout.channel.close()
          break    # exit as remote side is finished and our bufferes are empty

  # close all the pseudofiles
  stdout.close()
  stderr.close()

  if want_exitcode:
      # exit code is always ready at this point
      return (''.join(stdout_chunks), stdout.channel.recv_exit_status())
  return ''.join(stdout_chunks)

def iperf_check(host):
    cmd = 'iperf3'
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host, username='ec2-user')
    stdin, stdout, stderr = ssh.exec_command(cmd)
    output = stderr.readlines()

    for line in output:
        if line.find('not found') != -1:
            print host

def iperf_server(host, port):
    cmd = 'sudo iperf3 -s -p %d > /dev/null 2>&1 &' % (port)
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host, username='ec2-user')
    stdin, stdout, stderr = ssh.exec_command(cmd)

#    print host, cmd, stdout.readlines()

def iperf_kill_server(host):
    cmd = 'sudo killall iperf3'
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host, username='ec2-user')
    stdin, stdout, stderr = ssh.exec_command(cmd)

def iperf_client(host, remote, port, time):
    cmd = 'sudo iperf3 -c %s -p %d -t %d' % (remote, port, time)
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host, username='ec2-user')
    cliOut = myexec(ssh, cmd)
    #stdin, stdout, stderr = ssh.exec_command(cmd)

    #print cmd

    #output = stdout.readlines()
    ##stdin.flush()

    #cliOut = ""
    #for line in output:
    #    cliOut = cliOut + line
    #if cliOut == "":
    #    print host

    result = Mininet._parseIperf(cliOut)
    threadLock.acquire()
    throughput.append(result)
    threadLock.release()

    ssh.close()

def check():
    for i in range(0, num_hosts):
        iperf_check(publicIP['hosts'][i])

def run_exp(time, hosts, exp):
    global num_hosts
    threads = []
    if exp == 'check':
        check()
        return

    if exp == 'all':
        for i in range(0, hosts ):
            for j in range(0, hosts):
                if i == j:
                    continue
                iperf_server(publicIP['hosts'][i], ports[j])
                if i not in port_mapping.keys():
                    port_mapping[i] = {}
                port_mapping[i][j] = ports[j]
        sleep(1)

        for j in range(0, hosts):
            for i in range(0, hosts):
                if i == j:
                    continue
                t = threading.Thread(target=iperf_client, args=(publicIP['hosts'][i], privateIP['hosts'][j], port_mapping[j][i], time, ))
                t.start()
                threads.append(t)

        print 'Waiting...'
        for t in threads:
            t.join()
    else:
        for i in range(0, hosts, 2):
            iperf_server(publicIP['hosts'][i], ports[0])
            if i not in port_mapping.keys():
                port_mapping[i] = {}
            port_mapping[i][i+1] = ports[0]
        sleep(1)

        for i in range(1, hosts, 2):
            t = threading.Thread(target=iperf_client, args=(publicIP['hosts'][i], privateIP['hosts'][i-1], port_mapping[i-1][i], time, ))
            t.start()
            threads.append(t)

        print 'Waiting...'
        for t in threads:
            t.join()


    for i in range(0, hosts):
        iperf_kill_server(publicIP['hosts'][i])

    for res in throughput:
        print res



def init():
    global num_hosts, ports
    base_port = 5201

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

    for i in range(0, num_hosts):
        ports.append(base_port+i)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--exp", help="experiment type (all or single)", default="single")
    parser.add_argument("--hosts", help="number of hosts", default="2")
    parser.add_argument("--time", help="iperf running time", default="10")
    args = parser.parse_args()
    init()
    run_exp(int(args.time), int(args.hosts), args.exp)


