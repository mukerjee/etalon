#!/bin/bash

sudo apt-get -y install software-properties-common
sudo add-apt-repository -y "ppa:patrickdk/general-lucid"
sudo apt-get update
sudo apt-get -y install autoconf automake libtool make gcc git socat psmisc xterm ssh iperf iperf3 iproute telnet python-setuptools cgroup-bin ethtool help2man pyflakes pylint pep8 git-core autotools-dev pkg-config libc6-dev python-numpy python-matplotlib

cd $HOME
git clone git://openflowswitch.org/openflow.git
cd $HOME/openflow
sudo ./boot.sh
sudo ./configure
patch -p1 < $HOME/sdrt/VirtualTimeForMininet/mininet/util/openflow-patches/controller.patch
sudo make
sudo make install

sudo apt-get -y install openvswitch-switch
sudo apt-get -y install openvswitch-controller
sudo service openvswitch-controller stop
if [ -e /etc/init.d/openvswitch-controller ]; then
   sudo update-rc.d openvswitch-controller disable
fi

cd /proj/dna-PG0/linux-3.16.3-vtmininet
# sudo make -j16
sudo make headers_install
# sudo make modules
sudo make modules_install
sudo make install

sudo sed -i -r 's/GRUB_DEFAULT=0/GRUB_DEFAULT=1/' /etc/default/grub
sudo update-grub

cd $HOME/sdrt/VirtualTimeForMininet/mininet
sudo make clean
sudo make install

sudo reboot
