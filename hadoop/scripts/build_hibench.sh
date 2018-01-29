#!/bin/bash

sudo apt-get update
sudo apt-get install -y maven openjdk-8-jdk
cd $HOME
git clone https://github.com/intel-hadoop/HiBench.git
cd HiBench/
cp /sdrt/vhost/config/hadoop.conf ./conf/
cp /sdrt/vhost/config/hibench.conf ./conf/
mvn -Phadoopbench -Dspark=2.1 -Dscala=2.11 clean package
cd $HOME
tar cfvz ./HiBench.tar.gz ./HiBench/
