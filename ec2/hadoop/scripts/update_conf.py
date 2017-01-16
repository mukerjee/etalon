#!/usr/bin/python

import subprocess
import paramiko
import argparse
import os

import re

publicIP = {'namenode':[], 'datanode':[]}
privateDNS = {'namenode':[], 'datanode':[]}
namenode_publicDNS = ''
num_datanode = 0

def run_cmd(host, cmd):
    print host, cmd
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host, username='ec2-user')
    stdin, stdout, stderr = ssh.exec_command(cmd)


def run_install():

    HADOOP_CONF_DIR = '/usr/local/hadoop/etc/hadoop/'
    HADOOP_INSTALL = '/usr/local/hadoop'
    cmd = 'scp -r ./conf/* ec2-user@%s:%s' % (publicIP['namenode'][0], HADOOP_CONF_DIR)
    os.system(cmd)
    cmd = 'scp ./id_rsa.pub ec2-user@%s:~/.ssh/' % (publicIP['namenode'][0])
    os.system(cmd)
    cmd = 'scp ./id_rsa ec2-user@%s:~/.ssh/' % (publicIP['namenode'][0])
    os.system(cmd)
    cmd = 'scp ./config ec2-user@%s:~/.ssh/' % (publicIP['namenode'][0])
    os.system(cmd)

    run_cmd(publicIP['namenode'][0], 'chmod 600 /home/ec2-user/.ssh/config')
    run_cmd(publicIP['namenode'][0], 'rm -rf %s/hadoop_data/hdfs/namenode' % (HADOOP_INSTALL))
    run_cmd(publicIP['namenode'][0], 'mkdir -p %s/hadoop_data/hdfs/namenode' % (HADOOP_INSTALL))
    run_cmd(publicIP['namenode'][0], 'touch %s/masters' % (HADOOP_CONF_DIR))
    run_cmd(publicIP['namenode'][0], 'echo \"%s\" > %s/masters' % (privateDNS['namenode'][0], HADOOP_CONF_DIR))

    run_cmd(publicIP['namenode'][0], 'rm %s/slaves' % (HADOOP_CONF_DIR))
    for i in range (0, num_datanode):
        run_cmd(publicIP['namenode'][0], 'echo \"%s\" >> %s/slaves' % (privateDNS['datanode'][i],HADOOP_CONF_DIR))

    run_cmd(publicIP['namenode'][0], 'sed -i \'s/placeholder/%s/g\' %s/mapred-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
    run_cmd(publicIP['namenode'][0], 'sed -i \'s/placeholder/%s/g\' %s/yarn-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
    run_cmd(publicIP['namenode'][0], 'sed -i \'s/placeholder/%s/g\' %s/core-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
    run_cmd(publicIP['namenode'][0], 'echo \'%s\t%s\' | sudo tee --append /etc/hosts'% (namenode_publicDNS, privateDNS['namenode'][0]))

    f = open('id_rsa.pub','r')
    for line in f:
        pub = line.rstrip('\r\n')
    run_cmd(publicIP['namenode'][0], 'echo \"%s\" >> /home/ec2-user/.ssh/authorized_keys' % (pub))
    run_cmd(publicIP['namenode'][0], 'git clone https://github.com/intel-hadoop/HiBench.git /home/ec2-user/HiBench')
    run_cmd(publicIP['namenode'][0], 'cd /home/ec2-user/HiBench; mvn -Phadoopbench clean package')

    for i in range (0, num_datanode):
        run_cmd(publicIP['datanode'][i], 'rm -rf /tmp')
        cmd = 'scp ./config ec2-user@%s:~/.ssh/' % (publicIP['datanode'][i])
        os.system(cmd)
        run_cmd(publicIP['datanode'][i], 'chmod 600 /home/ec2-user/.ssh/config')
        cmd = 'scp -r ./conf/* ec2-user@%s:%s' % (publicIP['datanode'][i], HADOOP_CONF_DIR)
        os.system(cmd)
        run_cmd(publicIP['datanode'][i], 'mkdir -p %s/hadoop_data/hdfs/datanode' % (HADOOP_INSTALL))
        run_cmd(publicIP['datanode'][i], 'echo \"%s\" >> /home/ec2-user/.ssh/authorized_keys' % (pub))
        run_cmd(publicIP['datanode'][i], 'sed -i \'s/placeholder/%s/g\' %s/mapred-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
        run_cmd(publicIP['datanode'][i], 'sed -i \'s/placeholder/%s/g\' %s/yarn-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
        run_cmd(publicIP['datanode'][i], 'sed -i \'s/placeholder/%s/g\' %s/core-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
        run_cmd(publicIP['datanode'][i], 'sed -i \'s/namenode/datanode/g\' %s/hdfs-site.xml' % (HADOOP_CONF_DIR))

    cmd = 'scp ./hadoop.conf ec2-user@%s:/home/ec2-user/HiBench/conf/' % (publicIP['namenode'][0])
    print cmd
    os.system(cmd)
    run_cmd(publicIP['namenode'][0], 'sed -i \'s/localhost/%s/g\' /home/ec2-user/HiBench/conf/hadoop.conf' %(namenode_publicDNS))
    run_cmd(publicIP['namenode'][0], 'hdfs namenode -format')

def init():
    global publicIP
    global privateDNS
    global num_datanode
    global namenode_publicDNS

    switch_idx = -1

    cmd = ("aws ec2 describe-instances "
           "--query \"Reservations[*].Instances[*].PublicIpAddress\" "
           "--output=text")

    output = subprocess.check_output(cmd, shell=True)
    ip_list = re.split('\r|\t|\n', output)
    start = False
    ip_list2 = []
    for ip in ip_list:
        if ip != '':
            ip_list2.append(ip)

    for i in range(0, len(ip_list2)):
        if ip_list2[i].startswith('3'):
            switch_idx = i
            continue
        else:
            if start == False:
                publicIP['namenode'] = [ip_list2[i]]
                start = True
            else:
                publicIP['datanode'].append(ip_list2[i])


    start = False
    cmd = ("aws ec2 describe-instances "
           "--query \"Reservations[*].Instances[*].PrivateDnsName\" "
           "--output=text")
    output = subprocess.check_output(cmd, shell=True)
    ip_list = re.split('\r|\t|\n', output)
    ip_list2 = []
    for ip in ip_list:
        if ip != '':
            ip_list2.append(ip)
    for i in range(0, len(ip_list2)):
        if i == switch_idx:
            continue
        else:
            ip_list2[i] = ip_list2[i].replace('.ec2.internal','')
            if start == False:
                privateDNS['namenode'] = [ip_list2[i]]
                start = True
            else:
                privateDNS['datanode'].append(ip_list2[i])

    cmd = ("aws ec2 describe-instances "
           "--query \"Reservations[*].Instances[*].PublicDnsName\" "
           "--output=text")
    output = subprocess.check_output(cmd, shell=True)
    ip_list = re.split('\r|\t|\n', output)
    name = publicIP['namenode'][0].replace('.','-')
    for i in range(0, len(ip_list)):
        if ip_list[i] == '':
            continue
        if i == switch_idx:
            continue
        if ip_list[i].find(name) != -1:
            namenode_publicDNS = ip_list[i]


    print namenode_publicDNS
    num_datanode = len(publicIP['datanode'])
    print 'Number of running datanodes: %d' % (num_datanode)

    print publicIP['namenode']
    print privateDNS


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    init()
    run_install()


