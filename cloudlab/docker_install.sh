#!/bin/bash

apt-get update
apt-get install openjdk-7-jdk maven

cd $HOME
git clone https://github.com/intel-hadoop/HiBench.git

cd $HOME
git clone https://github.com/flowgrind/flowgrind.git
cd flowgrind
autoreconf -i
./configure
make

cd $HOME/sdrt/sdrt-ctrl/lib
make
