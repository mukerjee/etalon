#!/bin/bash

OFED_VERSION=4.1-1.0.2.0
DPDK_VERSION=16.11_2.3

(crontab -l 2>/dev/null; echo "@reboot $HOME/sdrt/cloudlab/tune.sh") | crontab -

sudo apt-get update && apt-get install -y \
                               git

echo "" >> $HOME/.bashrc
echo "export SWITCH=1" >> $HOME/.bashrc

# Mellanox OFED
# http://www.mellanox.com/related-docs/prod_software/Mellanox_OFED_Linux_User_Manual_v4.0.pdf
cd $HOME
wget http://www.mellanox.com/downloads/ofed/MLNX_OFED-$OFED_VERSION/MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz
tar xfz ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz
sudo ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64/mlnxofedinstall --dpdk

# Mellanox DPDK
# http://www.mellanox.com/related-docs/prod_software/MLNX_DPDK_Quick_Start_Guide_v16.11_2.3.pdf
cd $HOME
sudo connectx_port_config -c eth,eth
echo 'options mlx4_core log_num_mgm_entry_size=-7' | sudo tee -a /etc/modprobe.d/mlx4.conf
sudo /etc/init.d/openibd restart
wget http://www.mellanox.com/downloads/Drivers/MLNX_DPDK_$DPDK_VERSION.tar.gz
tar xfz ./MLNX_DPDK_$DPDK_VERSION.tar.gz
cd ./MLNX_DPDK_$DPDK_VERSION
make install T=x86_64-native-linuxapp-gcc

# Huge pages
# http://dpdk.org/doc/guides-16.04/linux_gsg/sys_reqs.html
cd $HOME
sudo sed -i -r 's/GRUB_CMDLINE_LINUX=\"(.*)\"/GRUB_CMDLINE_LINUX=\"\1 default_hugepagesz=1G hugepagesz=1G hugepages=4\"/' /etc/default/grub
sudo update-grub
sudo mkdir /mnt/huge_1GB
echo 'nodev /mnt/huge_1GB hugetlbfs pagesize=1GB 0 0' | sudo tee -a /etc/fstab

echo "" >> $HOME/.bashrc
echo "export RTE_SDK=$HOME/MLNX_DPDK_$DPDK_VERSION" >> $HOME/.bashrc
echo "export RTE_TARGET=x86_64-native-linuxapp-gcc" >> $HOME/.bashrc
export RTE_SDK=$HOME/MLNX_DPDK_$DPDK_VERSION
export RTE_TARGET=x86_64-native-linuxapp-gcc

# make Click
cd $HOME/sdrt/click-sdrt
./configure --enable-user-multithread --disable-linuxmodule --enable-intel-cpu --enable-nanotimestamp --enable-dpdk
make -j

# get SDRT
cd $HOME
git clone --recursive https://github.com/mukerjee/sdrt.git

sudo reboot
