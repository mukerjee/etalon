#!/bin/bash

OFED_VERSION=4.1-1.0.2.0

# get SDRT
cd /local
GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no" git clone --recursive https://github.com/mukerjee/sdrt.git
(crontab -l 2>/dev/null; echo "@reboot sleep 60 && /local/sdrt/cloudlab/tune.sh") | crontab -
sudo rm /var/run/crond.reboot

sudo apt-get update && sudo apt-get install -y \
                            git

# Mellanox OFED
# http://www.mellanox.com/related-docs/prod_software/Mellanox_OFED_Linux_User_Manual_v4.0.pdf
cd /local
wget http://www.mellanox.com/downloads/ofed/MLNX_OFED-$OFED_VERSION/MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz
tar xfz ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz
sudo ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64/mlnxofedinstall --force
sudo connectx_port_config -c eth,eth
sudo /etc/init.d/openibd restart

# get docker
cd /local
curl -fsSL get.docker.com -o get-docker.sh
sudo sh get-docker.sh
for u in `cut -d: -f1 /etc/passwd`
do
    sudo usermod -aG docker $u
done

# get pipework
cd /local
sudo bash -c "curl https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework > /usr/local/bin/pipework"
sudo chmod +x /usr/local/bin/pipework

# start in /local/sdrt/
for f in /users/*/.bashrc
do
    echo "" | sudo tee -a $f
    echo "cd /local/sdrt/" | sudo tee -a $f
done

/local/sdrt/cloudlab/tune.sh
