#!/bin/bash

sudo apt update &&
sudo apt install -y git tmux htop &&

# Install updated kernel.
sudo sed -i '/^# deb-src /s/^#//' /etc/apt/sources.list &&
sudo apt update &&
sudo apt -y build-dep linux-image-$(uname -r) &&
sudo apt install linux-cloud-tools-common linux-tools-common kernel-wedge &&

# Create the home directory environment, and mount a disk on which to build the
# kernel (the root device does not have sufficient space).
sudo mkfs.ext4 /dev/sda4 &&
mkdir $HOME/mnt &&
sudo mount /dev/sda4 $HOME/mnt
sudo chown -R `whoami`:dna-PG0 $HOME/mnt
sudo chown -R `whoami`:dna-PG0 $HOME/.config

# Apply the kernel patch, and compile.
git clone git://kernel.ubuntu.com/ubuntu/ubuntu-xenial.git $HOME/mnt/ubuntu-xenial &&
cd $HOME/mnt/ubuntu-xenial &&
git apply $HOME/etalon/reTCP/kernel-patch.patch &&
fakeroot debian/rules clean &&
MAKEFLAGS="-j 16" fakeroot debian/rules binary-headers binary-generic binary-perarch &&
sudo dpkg -i ../*.deb &&
rm -rf ubuntu-xenial &&

sudo reboot
