#!/bin/bash
#
# Set up an Etalon switch machine.

set -o errexit

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
     iptables-persistent \
     lib32z1-dev \
     libcurl4-gnutls-dev \
     libnuma-dev \
     libxmlrpc-core-c3-dev \
     maven \
     openjdk-8-jdk \
     uuid-dev

cd /etalon
git submodule update --init

# Add entries to the FORWARD iptable to enable Linux IP forwarding for the
# emulated hosts. This is not required. but is useful for debugging. This allows
# us to disable the Etalon hybrid switch without breaking connectivity between
# the emulated hosts. Otherwise, to send traffic between the emulated hosts
# without using the Etalon hybrid switch, we would need to modify the emulated
# hosts' ARP tables to remove the poison records that divert traffic destined
# for other emulated hosts to the switch machine.
for i in `seq 1 $NUM_RACKS`; do
    for j in `seq 1 $HOSTS_PER_RACK`; do
        sudo iptables -I FORWARD -s 10.$DATA_NET.$j.$j -j ACCEPT
    done
done
sudo mkdir -p /etc/iptables
echo `sudo iptables-save` | sudo tee /etc/iptables/rules.v4

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
sudo rm -rfv /mnt/huge_1GB
sudo mkdir -v /mnt/huge_1GB
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
# Copy to the dir in which the docker build will run.
cp -fv `which flowgrindd` /etalon/vhost/

# libVT.
echo "Installing libVT..."
cd /etalon/libVT
sudo make -j `nproc` install
# Copy to the dir in which the docker build will run.
cp -fv libVT.so /etalon/vhost/

# Set up SSH keys.
echo "Setting up SSH..."
cp -fv /etalon/vhost/config/ssh/id_rsa $HOME/.ssh/
cp -fv /etalon/vhost/config/ssh/id_rsa.pub $HOME/.ssh/
chmod 600 $HOME/.ssh/id_rsa
chmod 600 $HOME/.ssh/id_rsa.pub

# Do this last because afterwards apt complains and prevents packages from being
# installed.
source /etalon/bin/kernel_install.sh

# Fix permissions of ~/.config. Do this last because something else is setting
# the owner to "root".
if [ -d $HOME/.config ]; then
    sudo chown -R `whoami`:`whoami` $HOME/.config
fi

echo "Done"
sudo reboot
