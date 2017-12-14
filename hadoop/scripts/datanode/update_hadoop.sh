#!/bin/sh

#$HADOOP_INSTALL/sbin/stop-dfs.sh
BASE=/home/ec2-user/bin
VERSION=2.7.3

rm -rf $BASE/hadoop-$VERSION.tar.*
wget -P $BASE http://gs16689.sp.cs.cmu.edu:8080/hadoop-$VERSION.tar.gz
tar xvzf $BASE/hadoop-$VERSION.tar.gz -C $BASE

rm -rf $HADOOP_INSTALL/*
cp -r $BASE/hadoop-$VERSION/* $HADOOP_INSTALL
rm -rf $BASE/hadoop-$VERSION
cp -r $BASE/hadoop/* $HADOOP_CONF_DIR


mkdir -p $HADOOP_INSTALL/hadoop_data/hdfs/datanode

