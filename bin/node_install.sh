#!/bin/bash

OFED_VERSION=4.1-1.0.2.0

sudo apt-get update && sudo apt-get install -y \
                            git \
                            python-pip \
                            linuxptp && \

sudo pip install rpyc && \

# get Etalon
cd $HOME && \
GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no" git clone --recursive https://github.com/mukerjee/etalon.git && \

cd / && \
sudo ln -s ~/etalon && \

(crontab -l 2>/dev/null; echo "@reboot sleep 60 && $HOME/etalon/bin/tune.sh") | crontab - && \
sudo rm /var/run/crond.reboot && \


# Mellanox OFED
# http://www.mellanox.com/related-docs/prod_software/Mellanox_OFED_Linux_User_Manual_v4.0.pdf
cd $HOME && \
wget http://www.mellanox.com/downloads/ofed/MLNX_OFED-$OFED_VERSION/MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz && \
tar xfz ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz && \
sudo ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64/mlnxofedinstall --force && \
sudo connectx_port_config -c eth,eth && \
sudo /etc/init.d/openibd restart && \

# get docker
cd $HOME && \
curl -fsSL get.docker.com -o get-docker.sh && \
sudo sh get-docker.sh && \

# get pipework
cd $HOME && \
sudo bash -c "curl https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework > /usr/local/bin/pipework" && \
sudo chmod +x /usr/local/bin/pipework && \

sudo systemctl enable /etalon/rpycd/rpycd.service && \

# PTP
printf 'slaveOnly\t\t1\n[enp8s0d1]\n' | sudo tee -a /etc/linuxptp/ptp4l.conf && \
sudo sed -i '/(PTP) service/a Requires=network.target\nAfter=network.target' /lib/systemd/system/ptp4l.service && \
sudo sed -i 's/ -i eth0//' /lib/systemd/system/ptp4l.service && \
sudo sed -i 's/-w -s eth0/-a -r/' /lib/systemd/system/phc2sys.service && \
sudo systemctl daemon-reload && \
sudo systemctl enable phc2sys.service && \
sudo systemctl disable ntp.service && \

sudo mkdir /mnt/hdfs && \
/usr/local/etc/emulab/mkextrafs.pl /mnt/hdfs && \
sudo chown `whoami` /mnt/hdfs && \

# install updated kernel
sudo sed -i '/^# deb-src /s/^#//' /etc/apt/sources.list && \
sudo apt-get update && \
sudo apt-get -y build-dep linux-image-$(uname -r) && \
sudo apt-get install linux-cloud-tools-common linux-tools-common && \

cd /mnt/hdfs && \
git clone git://kernel.ubuntu.com/ubuntu/ubuntu-xenial.git && \
cd ubuntu-xenial && \
git apply /etalon/reTCP/kernel-patch.patch && \
fakeroot debian/rules clean && \
MAKEFLAGS="-j 16" fakeroot debian/rules binary-headers binary-generic binary-perarch && \
sudo dpkg -i ../*.deb && \

# move docker dir
sudo service docker stop && \
sudo mv /var/lib/docker /mnt/hdfs/ && \
sudo ln -s /mnt/hdfs/docker /var/lib/docker && \
sudo service docker start && \

sudo reboot
