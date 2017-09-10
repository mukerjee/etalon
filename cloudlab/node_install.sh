#!/bin/bash

sudo apt-get -y install software-properties-common
sudo add-apt-repository -y "ppa:patrickdk/general-lucid"
sudo apt-get update
sudo apt-get -y install iperf3 git zsh curl vim tmux python-pip xorg-dev libx11-dev htop git make g++ gcc emacs libcap-dev libidn11-dev nettle-dev autoconf automake libtool make gcc git socat psmisc xterm ssh iperf iperf3 iproute telnet python-setuptools cgroup-bin ethtool help2man pyflakes pylint pep8 git-core autotools-dev pkg-config libc6-dev python-numpy python-matplotlib nuttcp

cd $HOME
git clone git://openflowswitch.org/openflow.git
cd $HOME/openflow
sudo ./boot.sh
sudo ./configure
patch -p1 < $HOME/sdrt/vt-mininet/mininet/util/openflow-patches/controller.patch
sudo make -j16
sudo make -j16 install

sudo apt-get -y install openvswitch-switch
sudo apt-get -y install openvswitch-controller
sudo service openvswitch-controller stop
if [ -e /etc/init.d/openvswitch-controller ]; then
   sudo update-rc.d openvswitch-controller disable
fi

cd `mount | grep proj | cut -f3 -d' '`/linux-3.16.3-vt-mininet
# sudo make -j16
sudo make -j16 headers_install
# sudo make -j16 modules
sudo make -j16 modules_install
sudo make -j16 install

sudo sed -i -r 's/GRUB_DEFAULT=0/GRUB_DEFAULT=1/' /etc/default/grub
sudo update-grub

cd $HOME/sdrt/vt-mininet/mininet
sudo make clean
sudo make install

cd $HOME/sdrt/iputils/
make

sudo reboot
