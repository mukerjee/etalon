#!/bin/bash

cd $HOME
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.13.3/linux-headers-4.13.3-041303_4.13.3-041303.201709200606_all.deb


wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.13.3/linux-headers-4.13.3-041303-generic_4.13.3-041303.201709200606_amd64.deb


wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.13.3/linux-image-4.13.3-041303-generic_4.13.3-041303.201709200606_amd64.deb

echo "Installing new kernel. Will be prompted to enter password"
sudo dpkg -i *.deb

rm -f *.deb
echo "Reboot the Machine and verify installation with uname -sr"

# sudo reboot
