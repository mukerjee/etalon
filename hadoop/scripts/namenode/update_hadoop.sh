#!/bin/sh

VERSION=2.7.3
$HADOOP_INSTALL/sbin/stop-dfs.sh

rm -rf hadoop-$VERSION.tar.*
wget http://gs16689.sp.cs.cmu.edu:8080/hadoop-$VERSION.tar.gz
tar xvzf hadoop-$VERSION.tar.gz

rm -rf $HADOOP_INSTALL/*
cp -r hadoop-$VERSION/* $HADOOP_INSTALL
rm -rf hadoop-$VERSION
cp -r hadoop/* $HADOOP_CONF_DIR

mkdir -p $HADOOP_INSTALL/hadoop_data/hdfs/namenode
hdfs namenode -format

