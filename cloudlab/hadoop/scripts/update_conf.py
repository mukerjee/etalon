#!/usr/bin/python

import subprocess
import paramiko
import argparse
import os
import re

#publicIP = {'namenode':['apt164.apt.emulab.net'], 'datanode':['apt146.apt.emulab.net', 'apt138.apt.emulab.net', 'apt156.apt.emulab.net']}
publicIP = {'namenode':['apt164.apt.emulab.net'], 'datanode':['apt146.apt.emulab.net']}
privateDNS = {'namenode':['cp-1'], 'datanode':['cp-2']}
namenode_publicDNS = 'apt164.apt.emulab.net'
num_datanode =  1

def run_cmd(host, cmd):
    print host, cmd
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host, username='dkim')
    stdin, stdout, stderr = ssh.exec_command(cmd)

def run_install():

    HADOOP_CONF_DIR = '/usr/local/hadoop/etc/hadoop/'
    HADOOP_INSTALL = '/usr/local/hadoop'
    cmd = 'scp -r ./conf/* @%s:%s' % (publicIP['namenode'][0], HADOOP_CONF_DIR)
    os.system(cmd)
    cmd = 'scp ./config dkim@%s:~/.ssh/' % (publicIP['namenode'][0])
    os.system(cmd)
    run_cmd(publicIP['namenode'][0], 'chmod 600 /users/dkim/.ssh/config')

    # Clean namenode directory
    run_cmd(publicIP['namenode'][0], 'rm -rf %s/hadoop_data/hdfs/namenode' % (HADOOP_INSTALL))
    run_cmd(publicIP['namenode'][0], 'mkdir -p %s/hadoop_data/hdfs/namenode' % (HADOOP_INSTALL))

    # Set master node info
    run_cmd(publicIP['namenode'][0], 'touch %s/masters' % (HADOOP_CONF_DIR))
    run_cmd(publicIP['namenode'][0], 'echo \"%s\" > %s/masters' % (privateDNS['namenode'][0], HADOOP_CONF_DIR))

    # Set slave nodes info
    run_cmd(publicIP['namenode'][0], 'rm %s/slaves' % (HADOOP_CONF_DIR))
    for i in range (0, num_datanode):
        run_cmd(publicIP['namenode'][0], 'echo \"%s\" >> %s/slaves' % (privateDNS['datanode'][i],HADOOP_CONF_DIR))

    # Set site info
    run_cmd(publicIP['namenode'][0], 'sed -i \'s/placeholder/%s/g\' %s/mapred-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
    run_cmd(publicIP['namenode'][0], 'sed -i \'s/placeholder/%s/g\' %s/yarn-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
    run_cmd(publicIP['namenode'][0], 'sed -i \'s/placeholder/%s/g\' %s/core-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
    run_cmd(publicIP['namenode'][0], 'echo \'%s\t%s\' | sudo tee --append /etc/hosts'% (namenode_publicDNS, privateDNS['namenode'][0]))

    #f = open('id_rsa.pub','r')
    #for line in f:
    #    pub = line.rstrip('\r\n')
    #run_cmd(publicIP['namenode'][0], 'echo \"%s\" >> /users/dkim/.ssh/authorized_keys' % (pub))

    # Download hibench
    run_cmd(publicIP['namenode'][0], 'git clone https://github.com/intel-hadoop/HiBench.git /users/dkim/HiBench')
    run_cmd(publicIP['namenode'][0], 'cd /users/dkim/HiBench; mvn -Phadoopbench clean package')

    # Set datanode
    for i in range (0, num_datanode):
        run_cmd(publicIP['datanode'][i], 'rm -rf /tmp')
        cmd = 'scp ./config dkim@%s:~/.ssh/' % (publicIP['datanode'][i])
        os.system(cmd)
        run_cmd(publicIP['datanode'][i], 'chmod 600 /users/dkim/.ssh/config')
        cmd = 'scp -r ./conf/* dkim@%s:%s' % (publicIP['datanode'][i], HADOOP_CONF_DIR)
        os.system(cmd)
        run_cmd(publicIP['datanode'][i], 'mkdir -p %s/hadoop_data/hdfs/datanode' % (HADOOP_INSTALL))
        run_cmd(publicIP['datanode'][i], 'echo \"%s\" >> /users/dkim/.ssh/authorized_keys' % (pub))
        run_cmd(publicIP['datanode'][i], 'sed -i \'s/placeholder/%s/g\' %s/mapred-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
        run_cmd(publicIP['datanode'][i], 'sed -i \'s/placeholder/%s/g\' %s/yarn-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
        run_cmd(publicIP['datanode'][i], 'sed -i \'s/placeholder/%s/g\' %s/core-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
        run_cmd(publicIP['datanode'][i], 'sed -i \'s/namenode/datanode/g\' %s/hdfs-site.xml' % (HADOOP_CONF_DIR))

    cmd = 'scp ./hadoop.conf dkim@%s:/users/dkim/HiBench/conf/' % (publicIP['namenode'][0])
    print cmd
    os.system(cmd)
    run_cmd(publicIP['namenode'][0], 'sed -i \'s/localhost/%s/g\' /users/dkim/HiBench/conf/hadoop.conf' %(namenode_publicDNS))

    # Format hdfs
    run_cmd(publicIP['namenode'][0], 'hdfs namenode -format')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    run_install()


