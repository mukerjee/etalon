#!/bin/bash
#
# Install software common to both the Etalon nodes and switch.

set -o errexit

UBUNTU_VERSION=18.04
OFED_VERSION=4.6-1.0.1.1

if [ -d $HOME/.config ]; then
    sudo chown -R `whoami`:`whoami` $HOME/.config
fi

sudo rm -rfv /etalon
sudo ln -sfv $HOME/etalon /etalon

sudo apt update
sudo apt install -y git linuxptp python-pip
sudo -H pip install numpy rpyc
sudo systemctl enable /etalon/rpycd/rpycd.service

# Mellanox OFED.
# https://docs.mellanox.com/display/MLNXOFEDv461000/Introduction
echo "Installing MLNX OFED..."
cd
# Download and mount ISO.
wget http://www.mellanox.com/downloads/ofed/MLNX_OFED-$OFED_VERSION/MLNX_OFED_LINUX-$OFED_VERSION-ubuntu$UBUNTU_VERSION-x86_64.iso
sudo mount -o ro,loop MLNX_OFED_LINUX-$OFED_VERSION-ubuntu$UBUNTU_VERSION-x86_64.iso /mnt
# Install.
sudo /mnt/mlnxofedinstall --kernel-only --force
sudo connectx_port_config -c eth,eth
echo 'options mlx4_core log_num_mgm_entry_size=-7' | sudo tee -a /etc/modprobe.d/mlx4.conf
sudo /etc/init.d/openibd restart
# Clean up, but keep the ISO as a record.
sudo umount /mnt

# Docker.
echo "Installing docker..."
curl -fsSL https://get.docker.com -o $HOME/get-docker.sh
sudo sh $HOME/get-docker.sh
rm -fv $HOME/get-docker.sh

# PTP.
echo "Setting up PTP..."
printf '[enp68s0]\n' | sudo tee -a /etc/linuxptp/ptp4l.conf
sudo sed -i '/(PTP) service/a Requires=network.target\nAfter=network.target' /lib/systemd/system/ptp4l.service
sudo sed -i 's/ -i eth0//' /lib/systemd/system/ptp4l.service
sudo sed -i 's/-w -s eth0/-c enp68s0 -s CLOCK_REALTIME -w/' /lib/systemd/system/phc2sys.service
sudo systemctl daemon-reload
sudo systemctl enable phc2sys.service
if systemctl list-unit-files | grep ntp.service; then
    sudo systemctl disable ntp.service
fi
