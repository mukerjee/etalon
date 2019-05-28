#!/bin/bash
#
# The first argument is the mode. "--local" implies a local cluster. Empty
# implies CloudLab.

set -o errexit

MODE=$1
echo "MODE: $MODE"
UBUNTU_VERSION=18.04
OFED_VERSION=4.6-1.0.1.1

if [ ! -d $HOME/etalon ]; then
    echo "Error: Etalon repo not located at \"$HOME/etalon\"!"
    exit 1
fi

sudo apt update
sudo apt install -y \
     git \
     linuxptp \
     python-pip &&
sudo pip install rpyc &&

sudo ln -sfv $HOME/etalon /etalon &&

(crontab -l 2>/dev/null; echo "@reboot sleep 60 && $HOME/etalon/bin/tune.sh") | crontab - &&
sudo rm -fv /var/run/crond.reboot &&

# Mellanox OFED.
# https://docs.mellanox.com/display/MLNXOFEDv461000/Introduction
cd &&
# Set up repo.
wget http://www.mellanox.com/downloads/ofed/MLNX_OFED-$OFED_VERSION/MLNX_OFED_LINUX-$OFED_VERSION-ubuntu$UBUNTU_VERSION-x86_64.iso &&
sudo mount -o ro,loop MLNX_OFED_LINUX-$OFED_VERSION-ubuntu$UBUNTU_VERSION-x86_64.iso /mnt &&
echo "deb file:/mnt/DEBS /" | sudo tee /etc/apt/sources.list.d/mlnx_ofed.list &&
wget -qO - http://www.mellanox.com/downloads/ofed/RPM-GPG-KEY-Mellanox | sudo apt-key add - &&
# Install.
sudo apt update &&
sudo apt install -y mlnx-ofed-all mlnx-ofed-dpdk &&
sudo connectx_port_config -c eth,eth &&
sudo /etc/init.d/openibd restart &&
# Clean up, but keep the ISO as a record.
sudo rm -fv /etc/apt/sources.list.d/mlnx_ofed.list &&
sudo umount /mnt &&

# Docker.
cd &&
curl -fsSL https://get.docker.com -o get-docker.sh &&
sudo sh get-docker.sh &&

# Pipework.
cd &&
sudo bash -c "curl https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework > /usr/local/bin/pipework" &&
sudo chmod +x /usr/local/bin/pipework &&

sudo systemctl enable /etalon/rpycd/rpycd.service &&

# PTP.
printf 'slaveOnly\t\t1\n[enp68s0]\n' | sudo tee -a /etc/linuxptp/ptp4l.conf &&
sudo sed -i '/(PTP) service/a Requires=network.target\nAfter=network.target' /lib/systemd/system/ptp4l.service &&
sudo sed -i 's/ -i eth0//' /lib/systemd/system/ptp4l.service &&
sudo sed -i 's/-w -s eth0/-a -r/' /lib/systemd/system/phc2sys.service &&
sudo systemctl daemon-reload &&
sudo systemctl enable phc2sys.service &&
sudo systemctl disable ntp.service &&

if [ $MODE != "--local" ]; then
    # Move docker dir.
    sudo service docker stop &&
    sudo mv /var/lib/docker /mnt/hdfs/ &&
    sudo ln -sfv /mnt/hdfs/docker /var/lib/docker &&
    sudo service docker start
fi &&

echo "Done"
# sudo reboot
