#!/bin/bash
#
# Install Mellanox OFED. The first argument is the path to the ISO file.

set -o errexit

ISO=$1

sudo mount -o ro,loop $ISO /mnt
echo "deb file:/mnt/DEBS /" | sudo tee /etc/apt/sources.list.d/mlnx_ofed.list
wget -qO - http://www.mellanox.com/downloads/ofed/RPM-GPG-KEY-Mellanox | sudo apt-key add -
sudo apt update
sudo apt install -y mlnx-ofed-all
