#!/bin/bash

cd /sdrt/vhost/
tar xfvz hadoop-2.7.5.tar.gz
sudo mv hadoop-2.7.5 /usr/local/hadoop
cp /sdrt/hadoop/config/* /usr/local/hadoop/etc/hadoop/
sudo mkdir -p /mnt/hdfs
/usr/local/etc/emulab/mkextrafs.pl /mnt/hdfs
sudo mkdir -p /mnt/hdfs/namenode
sudo mkdir -p /mnt/hdfs/datanode

