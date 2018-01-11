#!/usr/bin/python

import subprocess
import paramiko
import argparse
import os
import re

VERSION='3.0.0-SNAPSHOT'
USERNAME = 'mukerjee'

publicIP = {'namenode':['10.1.10.1'], 'datanode':['10.1.10.2', '10.1.10.3', '10.1.10.4']}
privateDNS = {'namenode':['10.1.10.1'], 'datanode':['10.1.10.2', '10.1.10.3', '10.1.10.4']}
namenode_publicDNS = '10.1.10.1'
num_datanode = 3

def run_cmd(host, cmd):
    print host, cmd
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host, username=USERNAME)
    stdin, stdout, stderr = ssh.exec_command(cmd)
    exit_status = stdout.channel.recv_exit_status()          # Blocking call
    if exit_status == 0:
        print ("Done")
    else:
        print stderr
        print("Error", exit_status)
    ssh.close()

def run_cluster():

    HADOOP_INSTALL = '/usr/local/hadoop'
    HADOOP_CONF_DIR = '/usr/local/hadoop/etc/hadoop'
    HADOOP_BIN = '/usr/local/hadoop/bin'
    HADOOP_SBIN = '/usr/local/hadoop/sbin'

    #Install hadoop
    run_cmd(publicIP['namenode'][0], '%s/start-dfs.sh'%HADOOP_SBIN)
    run_cmd(publicIP['namenode'][0], '%s/start-yarn.sh'%HADOOP_SBIN)
    run_cmd(publicIP['namenode'][0], '%s/mr-jobhistory-daemon.sh start historyserver'%HADOOP_SBIN)

    # Set datanode
    #for i in range (0, num_datanode):
    #    addr = publicIP['datanode'][i].split('.')[3]
    #    if (addr == '12' or addr[1] == '1'):
    #        #it's already running
    #        continue
    #    print 're-start datanode %d'%i
    #    run_cmd(publicIP['datanode'][i], '%s/run-additionalDN.sh start 1' % (HADOOP_INSTALL))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    run_cluster()


