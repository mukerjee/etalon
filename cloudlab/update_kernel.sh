#!/bin/bash

cd $HOME
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.11.1/linux-headers-4.11.1-041101_4.11.1-041101.201705140931_all.deb


wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.11.1/linux-headers-4.11.1-041101-generic_4.11.1-041101.201705140931_amd64.deb


wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.11.1/linux-image-4.11.1-041101-generic_4.11.1-041101.201705140931_amd64.deb

echo "Installing new kernel. Will be prompted to enter password"
sudo dpkg -i *.deb

rm -f *.deb
echo "Reboot the Machine and verify installation with uname -sr"

sudo reboot
