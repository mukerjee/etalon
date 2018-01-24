#!/bin/bash

echo "export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64" >> $HOME/.bashrc
echo "export HADOOP_INSTALL=/usr/local/hadoop" >> $HOME/.bashrc
echo "export PATH=\$PATH:\$HADOOP_INSTALL/bin" >> $HOME/.bashrc
echo "export PATH=\$PATH:\$HADOOP_INSTALL/sbin" >> $HOME/.bashrc
echo "export HADOOP_MAPRED_HOME=\$HADOOP_INSTALL" >> $HOME/.bashrc
echo "export HADOOP_COMMON_HOME=\$HADOOP_INSTALL" >> $HOME/.bashrc
echo "export HADOOP_HDFS_HOME=\$HADOOP_INSTALL" >> $HOME/.bashrc
echo "export YARN_HOME=\$HADOOP_INSTALL" >> $HOME/.bashrc
echo "export HADOOP_COMMON_LIB_NATIVE_DIR=$HADOOP_INSTALL/lib/native" >> $HOME/.bashrc
echo "export HADOOP_OPTS="-Djava.library.path=$HADOOP_INSTALL/lib"" >> $HOME/.bashrc
echo "export HADOOP_CONF_DIR=/usr/local/hadoop/etc/hadoop" >> $HOME/.bashrc
