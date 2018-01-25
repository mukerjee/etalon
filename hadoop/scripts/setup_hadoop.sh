#!/bin/bash

sudo rm -rf /usr/local/hadoop
sudo rm -rf /tmp/*
sudo rm -rf /mnt/hdfs/*

cd /sdrt/
git pull

cd /sdrt/vhost/
tar xfz hadoop-$12.7.5.tar.gz
sudo mv hadoop-2.7.5 /usr/local/hadoop
cp /sdrt/hadoop/config/* /usr/local/hadoop/etc/hadoop/
sudo mkdir -p /mnt/hdfs
/usr/local/etc/emulab/mkextrafs.pl /mnt/hdfs
sudo mkdir -p /mnt/hdfs/namenode
sudo mkdir -p /mnt/hdfs/datanode
sudo mkdir -p /mnt/hdfs/tmp
sudo chmod 777 /mnt/hdfs/namenode
sudo chmod 777 /mnt/hdfs/datanode
sudo chmod 777 /mnt/hdfs/tmp
sudo chmod 777 /mnt/hdfs
