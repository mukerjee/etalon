#!/bin/bash

OFED_VERSION=4.1-1.0.2.0

# get SDRT
cd $HOME
GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no" git clone https://github.com/mukerjee/sdrt.git
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

printf 'slaveOnly\t\t1\n[enp8s0d1]\n' | sudo tee -a /etc/linuxptp/ptp4l.conf
sudo sed -i '/(PTP) service/a Requires=network.target\nAfter=network.target' /lib/systemd/system/ptp4l.service
sudo sed -i 's/ -i eth0//' /lib/systemd/system/ptp4l.service
sudo sed -i 's/-w -s eth0/-a -r/' /lib/systemd/system/phc2sys.service
sudo systemctl daemon-reload
sudo systemctl enable phc2sys.service
sudo systemctl disable ntp.service

sudo reboot
