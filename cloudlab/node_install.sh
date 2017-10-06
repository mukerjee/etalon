#!/bin/bash

OFED_VERSION=4.1-1.0.2.0

(crontab -l 2>/dev/null; echo "@reboot $HOME/sdrt/cloudlab/tune.sh") | crontab -

sudo apt-get update && sudo apt-get install -y \
                            git


# Mellanox OFED
# http://www.mellanox.com/related-docs/prod_software/Mellanox_OFED_Linux_User_Manual_v4.0.pdf
cd $HOME
wget http://www.mellanox.com/downloads/ofed/MLNX_OFED-$OFED_VERSION/MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz
tar xfz ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz
sudo ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64/mlnxofedinstall --force
sudo /etc/init.d/openibd restart

# get docker
cd $HOME
curl -fsSL get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# get pipework
cd $HOME
sudo bash -c "curl https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework > /usr/local/bin/pipework"
sudo chmod +x /usr/local/bin/pipework

# get SDRT
cd $HOME
GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no" git clone --recursive https://github.com/mukerjee/sdrt.git

sudo reboot
