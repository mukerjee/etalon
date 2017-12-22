#!/bin/bash

OFED_VERSION=4.1-1.0.2.0

# get SDRT
cd $HOME
GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no" git clone https://github.com/mukerjee/sdrt.git
GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no" git clone https://github.com/mukerjee/libVT.git
(crontab -l 2>/dev/null; echo "@reboot sleep 60 && $HOME/sdrt/cloudlab/tune.sh") | crontab -
sudo rm /var/run/crond.reboot

sudo apt-get update && sudo apt-get install -y \
                            git \
                            python-pip \
                            linuxptp

sudo pip install rpyc

# Mellanox OFED
# http://www.mellanox.com/related-docs/prod_software/Mellanox_OFED_Linux_User_Manual_v4.0.pdf
cd $HOME
wget http://www.mellanox.com/downloads/ofed/MLNX_OFED-$OFED_VERSION/MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz
tar xfz ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz
sudo ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64/mlnxofedinstall --force
sudo connectx_port_config -c eth,eth
sudo /etc/init.d/openibd restart

# get docker
cd $HOME
curl -fsSL get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker `whoami`

# get pipework
sudo bash -c "curl https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework > /usr/local/bin/pipework"
sudo chmod +x /usr/local/bin/pipework

sudo ln -s $HOME/sdrt /local
sudo systemctl enable $HOME/sdrt/cloudlab/rpyc_daemon.service

# PTP
printf 'slaveOnly\t\t1\n[enp8s0d1]\n' | sudo tee -a /etc/linuxptp/ptp4l.conf
sudo sed -i '/(PTP) service/a Requires=network.target\nAfter=network.target' /lib/systemd/system/ptp4l.service
sudo sed -i 's/ -i eth0//' /lib/systemd/system/ptp4l.service
sudo sed -i 's/-w -s eth0/-a -r/' /lib/systemd/system/phc2sys.service
sudo systemctl daemon-reload
sudo systemctl enable phc2sys.service
sudo systemctl disable ntp.service

# libVT
cd $HOME/libVT
sudo make install


# install updated kernel
# sudo mkdir /mnt/extra
# sudo /usr/local/etc/emulab/mkextrafs.pl /mnt/extra
# cd /mnt
# sudo chown `whoami` extra
# cd extra
# git clone git://kernel.ubuntu.com/ubuntu/ubuntu-xenial.git
# cd ubuntu-xenial
# sudo sed -i '/^# deb-src /s/^#//' /etc/apt/sources.list
# sudo apt-get update
# sudo apt-get -y build-dep linux-image-$(uname -r)
# git apply $HOME/sdrt/tcp_reno_tuner/kernel-patch.patch
# fakeroot debian/rules clean
# MAKEFLAGS="-j 16" fakeroot debian/rules binary-headers binary-generic binary-perarch
# sudo apt-get install linux-cloud-tools-common linux-tools-common
# sudo dpkg -i ../*.deb

sudo sed -i '/^# deb-src /s/^#//' /etc/apt/sources.list
sudo apt-get update
sudo apt-get -y build-dep linux-image-$(uname -r)
sudo apt-get install linux-cloud-tools-common linux-tools-common
sudo dpkg -i /proj/dna-PG0/sdrt/*.deb

cd /
sudo ln -s ~/sdrt

sudo reboot
