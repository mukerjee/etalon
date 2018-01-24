#!/bin/bash

cd /sdrt/vhost/
tar xfvz hadoop-2.7.5.tar.gz
sudo mv hadoop-2.7.5 /usr/local/hadoop
cp ./config/hadoop_config/* /usr/local/hadoop/etc/hadoop/
sudo mkdir -p /usr/local/hadoop/hadoop_data/hdfs/namenode
sudo mkdir -p /usr/local/hadoop/hadoop_data/hdfs/datanode

