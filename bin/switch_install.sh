#!/bin/bash
#
# Set up an Etalon switch machine.

set -o errexit

DPDK_VERSION=17.08.2

# Validate.
if [ ! -d $HOME/etalon ]; then
    echo "Error: Etalon repo not located at \"$HOME/etalon\"!"
    exit 1
fi

source $HOME/etalon/bin/common_install.sh "switch"

sudo apt update
sudo apt install -y \
     autoconf \
     cmake \
     lib32z1-dev \
     libcurl4-gnutls-dev \
     libnuma-dev \
     libxmlrpc-core-c3-dev \
     maven \
     openjdk-8-jdk \
     uuid-dev

cd /etalon
git submodule update --init

# Mellanox DPDK.
# http://www.mellanox.com/related-docs/prod_software/MLNX_DPDK_Quick_Start_Guide_v16.11_2.3.pdf
echo "Installing Mellanox DPDK..."
sudo apt install -y dpdk dpdk-dev

# Huge pages. http://dpdk.org/doc/guides/linux_gsg/sys_reqs.html
echo "Setting up huge pages..."
if mount | grep "/mnt/huge_1GB "; then
    sudo umount /mnt/huge_1GB
fi
# Configure huge pages to be allocated on boot.
sudo sed -i -r 's/GRUB_CMDLINE_LINUX=\"(.*)\"/GRUB_CMDLINE_LINUX=\"\1 default_hugepagesz=1G hugepagesz=1G hugepages=4\"/' /etc/default/grub
sudo update-grub
sudo rm -rf /mnt/huge_1GB
sudo mkdir /mnt/huge_1GB
echo 'nodev /mnt/huge_1GB hugetlbfs pagesize=1GB 0 0' | sudo tee -a /etc/fstab

# RTE_SDK location.
echo "Setting RTE_SDK location..."
echo "" >> $HOME/.bashrc
echo "export RTE_SDK=/usr/share/dpdk" >> $HOME/.bashrc
echo "export RTE_TARGET=x86_64-default-linuxapp-gcc" >> $HOME/.bashrc
export RTE_SDK=/usr/share/dpdk
export RTE_TARGET=x86_64-default-linuxapp-gcc

# Click.
echo "Installing Click..."
cd /etalon/click-etalon
./configure --enable-user-multithread --disable-linuxmodule --enable-intel-cpu --enable-nanotimestamp --enable-dpdk
make -j `nproc`
# "make install" needs gzcat.
WHICH_ZCAT=`which zcat`
sudo ln -sfv $WHICH_ZCAT `dirname $WHICH_ZCAT`/gzcat
sudo make -j `nproc` install

# Flowgrind.
echo "Installing Flowgrind..."
cd /etalon/flowgrind-etalon
autoreconf -i
./configure
make -j `nproc`
sudo make -j `nproc` install
cp -v /usr/local/sbin/flowgrindd /etalon/vhost/

# libVT.
echo "Installing libVT..."
cd /etalon/libVT
sudo make -j `nproc` install
sudo cp -v ./libVT.so /etalon/vhost/

# vhost SSH.
echo "Setting up SSH..."
cp /etalon/vhost/config/ssh/id_rsa $HOME/.ssh/
cp /etalon/vhost/config/ssh/id_rsa.pub $HOME/.ssh/
chmod 600 $HOME/.ssh/id_rsa
chmod 600 $HOME/.ssh/id_rsa.pub

echo "Done"
sudo reboot
