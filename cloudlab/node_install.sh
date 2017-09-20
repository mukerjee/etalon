#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root"
    exit 1
fi

sudo apt-get -y install software-properties-common
sudo add-apt-repository -y "ppa:patrickdk/general-lucid"
sudo apt-get update
sudo apt-get -y install iperf3 git zsh curl vim tmux python-pip xorg-dev libx11-dev htop git make g++ gcc emacs libcap-dev libidn11-dev nettle-dev autoconf automake libtool make gcc git socat psmisc xterm openjdk-7-jdk ssh iperf iperf3 iproute telnet python-setuptools cgroup-bin ethtool help2man pyflakes pylint pep8 git-core autotools-dev pkg-config libc6-dev python-numpy python-matplotlib nuttcp maven uuid-dev libcurl4-gnutls-dev libxmlrpc-core-c3-dev

sudo easy_install rpyc
printf "\nsudo ./sdrt/cloudlab/tune.sh\n" >> $HOME/.bashrc
printf "\nalias ping='sudo ping'\n"


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

cd $HOME
git clone https://github.com/intel-hadoop/HiBench.git

cd $HOME
git clone https://github.com/flowgrind/flowgrind.git
cd flowgrind
autoreconf -i
./configure
make

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
sudo ln -s $HOME/sdrt/iputils/ping /usr/local/bin/ping 2>/dev/null

cd $HOME/sdrt/sdrt-ctrl/lib
make

# cd $HOME/sdrt/iperf-2.0.10/
# ./configure
# make
# sudo ln -s $HOME/sdrt/iperf-2.0.10/src/iperf /usr/local/bin/iperf 2>/dev/null

if ! grep -q "apt.emulab" ~/.ssh/authorized_keys
then
    cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys
fi

sudo reboot
