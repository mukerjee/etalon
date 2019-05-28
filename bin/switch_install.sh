#!/bin/bash
#
# The first argument is the mode. "--local" implies a local cluster. Empty
# implies CloudLab.

set -o errexit

MODE=$1
echo "MODE: $MODE"
UBUNTU_VERSION=18.04
OFED_VERSION=4.6-1.0.1.1
DPDK_VERSION=17.08.2

if [ ! -d ~/etalon ]; then
    echo "Error: Etalon repo not located at \"~/etalon\"!"
    exit 1
fi
# If /mnt or /mnt/huge_1GB are mounted, then unmount them.
if mount | grep "/mnt "; then
    sudo umount /mnt
elif mount | grep "/mnt/huge_1GB "; then
    sudo umount /mnt/huge_1GB
fi &&

sudo apt update
sudo apt install -y \
     autoconf \
     cmake \
     git \
     lib32z1-dev \
     libcurl4-gnutls-dev \
     libxmlrpc-core-c3-dev \
     libnuma-dev \
     linuxptp \
     maven \
     openjdk-8-jdk \
     python-pip \
     uuid-dev &&
sudo pip install numpy rpyc &&

# get Etalon - Assume we downloaded it manually.
# cd &&
# GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no" git clone --recursive https://github.com/ccanel/etalon.git &&

cd / &&
sudo ln -sf ~/etalon &&
cd /etalon &&
git submodule update --init

(crontab -l 2>/dev/null; echo "@reboot sleep 60 && /etalon/bin/tune.sh") | crontab - &&
sudo rm -f /var/run/crond.reboot &&

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
echo 'options mlx4_core log_num_mgm_entry_size=-7' | sudo tee -a /etc/modprobe.d/mlx4.conf &&
sudo /etc/init.d/openibd restart &&
# Clean up, but keep the ISO as a record.
sudo rm -fv /etc/apt/sources.list.d/mlnx_ofed.list &&
sudo umount /mnt &&

# Mellanox DPDK.
# http://www.mellanox.com/related-docs/prod_software/MLNX_DPDK_Quick_Start_Guide_v16.11_2.3.pdf
echo "Installing Mellanox DPDK..." &&
cd &&
wget http://fast.dpdk.org/rel/dpdk-$DPDK_VERSION.tar.xz &&
tar xf dpdk-$DPDK_VERSION.tar.xz &&
rm -fv dpdk-$DPDK_VERSION.tar.xz &&  # Keep the untarred files as a record.
cd ./dpdk-stable-$DPDK_VERSION &&
make -j `nproc` install T=x86_64-native-linuxapp-gcc &&

# Huge pages. http://dpdk.org/doc/guides/linux_gsg/sys_reqs.html
echo "Setting up huge pages..." &&
# Configure huge pages to be allocated on boot.
sudo sed -i -r 's/GRUB_CMDLINE_LINUX=\"(.*)\"/GRUB_CMDLINE_LINUX=\"\1 default_hugepagesz=1G hugepagesz=1G hugepages=4\"/' /etc/default/grub &&
sudo update-grub &&
sudo rm -rf /mnt/huge_1GB &&
sudo mkdir /mnt/huge_1GB &&
echo 'nodev /mnt/huge_1GB hugetlbfs pagesize=1GB 0 0' | sudo tee -a /etc/fstab &&

# RTE_SDK location.
echo "Setting RTE_SDK location..." &&
echo "" >> $HOME/.bashrc &&
echo "export RTE_SDK=$HOME/dpdk-stable-$DPDK_VERSION" >> $HOME/.bashrc &&
echo "export RTE_TARGET=x86_64-native-linuxapp-gcc" >> $HOME/.bashrc &&
export RTE_SDK=$HOME/dpdk-stable-$DPDK_VERSION &&
export RTE_TARGET=x86_64-native-linuxapp-gcc &&

# Click.
echo "Installing Click..." &&
cd /etalon/click-etalon &&
./configure --enable-user-multithread --disable-linuxmodule --enable-intel-cpu --enable-nanotimestamp --enable-dpdk &&
make -j `nproc` &&
sudo make -j `nproc` install &&

# Flowgrind.
echo "Installing flowgrind..." &&
cd /etalon/flowgrind-etalon &&
autoreconf -i &&
./configure &&
make -j `nproc` &&
sudo make -j `nproc` install &&
cp /usr/local/sbin/flowgrindd /etalon/vhost/ &&

# libVT.
echo "Installing libVT..." &&
cd /etalon/libVT &&
sudo make -j `nproc` install &&
sudo cp ./libVT.so /etalon/vhost/ &&

# libADU - Ignoring for now.
# echo "Installing libADU..." &&
# cd /etalon/libADU &&
# sudo make -j `nproc` install &&

# Docker.
echo "Installing docker..." &&
cd &&
curl -fsSL https://get.docker.com -o get-docker.sh &&
sudo sh get-docker.sh &&
rm $HOME/get-docker.sh &&

# PTP.
echo "Setting up PTP..." &&
printf '[enp68s0]\n' | sudo tee -a /etc/linuxptp/ptp4l.conf &&
sudo sed -i '/(PTP) service/a Requires=network.target\nAfter=network.target' /lib/systemd/system/ptp4l.service &&
sudo sed -i 's/ -i eth0//' /lib/systemd/system/ptp4l.service &&
sudo sed -i 's/-w -s eth0/-c enp68s0 -s CLOCK_REALTIME -w/' /lib/systemd/system/phc2sys.service &&
sudo systemctl daemon-reload &&
sudo systemctl enable phc2sys.service &&
sudo systemctl disable ntp.service &&

# vhost SSH.
echo "Setting up SSH..." &&
cp /etalon/vhost/config/ssh/id_rsa $HOME/.ssh/ &&
cp /etalon/vhost/config/ssh/id_rsa.pub $HOME/.ssh/ &&
chmod 600 $HOME/.ssh/id_rsa &&
chmod 600 $HOME/.ssh/id_rsa.pub &&

# HiBench - Ignoring for now.
# echo "Installing HiBench..." &&
# cd &&
# git clone https://github.com/intel-hadoop/HiBench.git &&
# cd HiBench/ &&
# mvn -Phadoopbench -Dspark=2.1 -Dscala=2.11 clean package &&
# rm -rf .git common docker docs flinkbench gearpumpbench hadoopbench sparkbench stormbench travis &&
# cd ../ &&
# tar cfvz ./HiBench.tar.gz ./HiBench/ &&
# rm -rf ./HiBench &&
# mv ./HiBench.tar.gz /etalon/vhost/ &&

# protobuff - Ignoring for now.
# echo "Installing protobuf..." &&
# cd &&
# wget https://github.com/google/protobuf/releases/download/v2.5.0/protobuf-2.5.0.tar.gz &&
# tar zxvf protobuf-2.5.0.tar.gz &&
# cd protobuf-2.5.0 &&
# ./configure &&
# make -j `nproc` &&
# make -j `nproc` check &&
# sudo make -j `nproc` install &&
# sudo ldconfig &&

# Hadoop 2.9 - Ignoring for now.
# echo "Installing Hadoop..." &&
# cd &&
# wget http://apache.mirrors.pair.com/hadoop/common/hadoop-2.9.0/hadoop-2.9.0-src.tar.gz &&
# tar xfvz hadoop-2.9.0-src.tar.gz &&
# cd hadoop-2.9.0-src &&
# cp /etalon/reHDFS/* ./hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/ &&
# mvn package -Pdist,native -DskipTests -Dtar &&
# cp ./hadoop-dist/target/hadoop-2.9.0.tar.gz /etalon/vhost/ &&
# rm -rf $HOME/hadoop-2.9.0-src &&

# Fix broken kill in 16.04 - Ignoring for now (we are using 18.04).
# echo "Fixing broken kill..." &&
# cd &&
# sudo sed -i -e 's/# deb-src/deb-src/' /etc/apt/sources.list &&
# sudo apt update &&
# sudo apt source procps &&
# sudo apt build-dep -y procps &&
# cd procps-3.3.10 &&
# sudo dpkg-buildpackage &&
# cp ./.libs/kill /etalon/vhost/ &&
# sudo rm -rf $HOME/libprocps* &&
# sudo rm -rf $HOME/procps* &&

if [ $MODE != "--local" ]; then
    # Get extra space - Ignoring for now.
    # echo "Getting extra space..." &&
    # cd &&
    # sudo mkdir /mnt/hdfs &&
    # sudo /usr/local/etc/emulab/mkextrafs.pl -f /mnt/hdfs &&
    # sudo chown `whoami` /mnt/hdfs &&

    # Move docker dir.
    echo "Moving docker dir..." &&
    sudo service docker stop &&
    sudo mv /var/lib/docker /mnt/hdfs/ &&
    sudo ln -s /mnt/hdfs/docker /var/lib/docker &&
    sudo service docker start
fi &&

echo "Done"
# sudo reboot
