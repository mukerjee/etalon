#!/bin/bash

sudo apt-get update && sudo apt-get install -y \
                            git &&

# install updated kernel
sudo sed -i '/^# deb-src /s/^#//' /etc/apt/sources.list &&
sudo apt-get update &&
sudo apt-get -y build-dep linux-image-$(uname -r) &&
sudo apt-get install linux-cloud-tools-common linux-tools-common &&

cd $HOME &&
git clone git://kernel.ubuntu.com/ubuntu/ubuntu-xenial.git &&
cd ubuntu-xenial &&
git apply $HOME/etalon/reTCP/kernel-patch.patch &&
fakeroot debian/rules clean &&
MAKEFLAGS="-j 16" fakeroot debian/rules binary-headers binary-generic binary-perarch &&
sudo dpkg -i ../*.deb &&
rm -rf ubuntu-xenial &&

sudo reboot
