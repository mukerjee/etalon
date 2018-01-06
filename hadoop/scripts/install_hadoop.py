#!/usr/bin/python

#import subprocess
import paramiko
import argparse
import os
#import re

VERSION='3.0.0'
USERNAME = 'mukerjee'

publicIP = {'namenode':['10.10.1.11'], 'datanode':['10.10.1.12', '10.10.1.21', '10.10.1.22']}
privateDNS = {'namenode':['10.10.1.11'], 'datanode':['10.10.1.12', '10.10.1.21', '10.10.1.22']}
namenode_publicDNS = '10.10.1.11'
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

def run_install():

    HADOOP_CONF_DIR = '/hadoop/etc/hadoop'
    HADOOP_INSTALL = '/hadoop'
    HADOOP_BIN = '/hadoop/bin'

    #Install hadoop
    run_cmd(publicIP['namenode'][0], 'pkill -9 java')
    run_cmd(publicIP['namenode'][0], 'rm -rf /hadoop/*')
    run_cmd(publicIP['namenode'][0], 'rm -rf /tmp/hadoop*')
    run_cmd(publicIP['namenode'][0], 'cp -r /users/'+USERNAME+'/hadoop-'+VERSION+'/* /hadoop/')

    cmd = 'scp -r ./conf/* '+USERNAME+'@%s:%s' % (publicIP['namenode'][0], HADOOP_CONF_DIR)
    os.system(cmd)
    cmd = 'scp ./config '+USERNAME+'@%s:~/.ssh/' % (publicIP['namenode'][0])
    os.system(cmd)
    run_cmd(publicIP['namenode'][0], 'chmod 600 /users/'+USERNAME+'/.ssh/config')

    # Clean namenode directory
    run_cmd(publicIP['namenode'][0], 'rm -rf %s/hadoop_data/hdfs/namenode' % (HADOOP_INSTALL))
    run_cmd(publicIP['namenode'][0], 'mkdir -p %s/hadoop_data/hdfs/namenode' % (HADOOP_INSTALL))

    # Set master node info
    run_cmd(publicIP['namenode'][0], 'rm -rf %s/masters' % (HADOOP_CONF_DIR))
    run_cmd(publicIP['namenode'][0], 'touch %s/masters' % (HADOOP_CONF_DIR))
    run_cmd(publicIP['namenode'][0], 'echo \"%s\" > %s/masters' % (privateDNS['namenode'][0], HADOOP_CONF_DIR))

    # Set slave nodes info
    run_cmd(publicIP['namenode'][0], 'rm %s/slaves' % (HADOOP_CONF_DIR))
    run_cmd(publicIP['namenode'][0], 'touch %s/slaves' % (HADOOP_CONF_DIR))
    for i in range (0, num_datanode):
        run_cmd(publicIP['namenode'][0], 'echo \"%s\" >> %s/slaves' % (privateDNS['datanode'][i],HADOOP_CONF_DIR))

    # Set site info
    run_cmd(publicIP['namenode'][0], 'sed -i \'s/placeholder/%s/g\' %s/mapred-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
    run_cmd(publicIP['namenode'][0], 'sed -i \'s/placeholder/%s/g\' %s/yarn-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
    run_cmd(publicIP['namenode'][0], 'sed -i \'s/placeholder/%s/g\' %s/core-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
    #run_cmd(publicIP['namenode'][0], 'echo \'%s\t%s\' | sudo tee --append /etc/hosts'% (namenode_publicDNS, privateDNS['namenode'][0]))

    #f = open('id_rsa.pub','r')
    #for line in f:
    #    pub = line.rstrip('\r\n')
    #run_cmd(publicIP['namenode'][0], 'echo \"%s\" >> /users/dkim/.ssh/authorized_keys' % (pub))

    # Set datanode
    for i in range (0, num_datanode):
        print 'Setup datanode %d'%i
        run_cmd(publicIP['datanode'][i], 'rm -rf /tmp/hadoop*')
        run_cmd(publicIP['datanode'][i], 'rm -rf /hadoop/*')
        run_cmd(publicIP['datanode'][i], 'pkill -9 java')
        run_cmd(publicIP['datanode'][i], 'cp -r /users/'+USERNAME+'/hadoop-'+VERSION+'/* /hadoop/')
        cmd = 'scp ./config '+USERNAME+'@%s:~/.ssh/' % (publicIP['datanode'][i])
        os.system(cmd)
        run_cmd(publicIP['datanode'][i], 'chmod 600 /users/'+USERNAME+'/.ssh/config')
        cmd = 'scp -r ./conf/* '+USERNAME+'@%s:%s' % (publicIP['datanode'][i], HADOOP_CONF_DIR)
        os.system(cmd)
        run_cmd(publicIP['datanode'][i], 'mkdir -p %s/hadoop_data/hdfs/datanode' % (HADOOP_INSTALL))
        #run_cmd(publicIP['datanode'][i], 'echo \"%s\" >> /users/dkim/.ssh/authorized_keys' % (pub))
        run_cmd(publicIP['datanode'][i], 'sed -i \'s/placeholder/%s/g\' %s/mapred-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
        run_cmd(publicIP['datanode'][i], 'sed -i \'s/placeholder/%s/g\' %s/yarn-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
        run_cmd(publicIP['datanode'][i], 'sed -i \'s/placeholder/%s/g\' %s/core-site.xml' % (namenode_publicDNS, HADOOP_CONF_DIR))
        run_cmd(publicIP['datanode'][i], 'sed -i \'s/namenode/datanode/g\' %s/hdfs-site.xml' % (HADOOP_CONF_DIR))

    cmd = 'scp ./hadoop.conf '+USERNAME+'@%s:/users/'+USERNAME+'/HiBench/conf/' % (publicIP['namenode'][0])
    os.system(cmd)
    run_cmd(publicIP['namenode'][0], 'sed -i \'s/localhost/%s/g\' /users/'+USERNAME+'/HiBench/conf/hadoop.conf' %(namenode_publicDNS))

    # Format hdfs
    run_cmd(publicIP['namenode'][0], '%s/hdfs namenode -format -force'%(HADOOP_BIN))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    run_install()


