#!/bin/bash

PWD=`pwd`
# install mininet deps
sudo apt-get install gcc make socat psmisc xterm ssh iproute telnet python-setuptools cgroup-bin ethtool help2man pyflakes pylint pep8

# install Open vSwitch
echo "---------------install Open vSwitch--------------------"
sudo ./$PWD/install.sh -v

# install OpenFlow
echo "-------------------install OpenFlow--------------------"
sudo ./$PWD/install.sh -f
#$install autoconf automake libtool make gcc git-core autotool-dev pkg-config libc6-dev
#if [ ! -d openflow-1.0.0 ];
#then tar xzvf openflow-1.0.0.tar.gz
#fi
#cd openflow-1.0.0
#patch -p1 < openflow-patches/controller.patch
#./boot.sh
#./configure
#make
#sudo make install
#cd ../

# maybe we need to build those software by ourself
echo "--------------------install my own iperf---------------"
pushd ../../iperf
./bootstrap.sh
./configure
sudo make
sudo make install
ldconfig
popd

echo "------------------install mininet-----------------------"
pushd ../
sudo make install
popd



