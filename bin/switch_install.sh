#!/bin/bash
#
# The first argument is the mode. "--local" implies a local cluster. Empty
# implies CloudLab.

set -o errexit

MODE=$1
echo "MODE: $MODE"
OFED_VERSION=4.1-1.0.2.0
DPDK_VERSION=16.11_2.3

sudo apt-get update && sudo apt-get install -y \
                            git \
                            python-pip \
                            linuxptp \
			    libcurl4-gnutls-dev \
			    libxmlrpc-core-c3-dev \
			    maven \
			    openjdk-8-jdk \
			    cmake &&
sudo pip install rpyc &&
sudo pip install numpy &&

# get Etalon - Assume we downloaded it manually.
# cd $HOME &&
# GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no" git clone --recursive https://github.com/ccanel/etalon.git &&

cd / &&
sudo ln -sf ~/etalon &&
cd /etalon &&
git submodule update --init

(crontab -l 2>/dev/null; echo "@reboot sleep 60 && /etalon/bin/tune.sh") | crontab - &&
sudo rm -f /var/run/crond.reboot &&


# Mellanox OFED - Assume that this has been installed manually for now.
# http://www.mellanox.com/related-docs/prod_software/Mellanox_OFED_Linux_User_Manual_v4.0.pdf
# cd $HOME &&
# wget http://www.mellanox.com/downloads/ofed/MLNX_OFED-$OFED_VERSION/MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz &&
# tar xfz ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz &&
# sudo ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64/mlnxofedinstall --force --dpdk &&

# Mellanox DPDK
# http://www.mellanox.com/related-docs/prod_software/MLNX_DPDK_Quick_Start_Guide_v16.11_2.3.pdf
cd $HOME &&
sudo connectx_port_config -c eth,eth &&
echo 'options mlx4_core log_num_mgm_entry_size=-7' | sudo tee -a /etc/modprobe.d/mlx4.conf &&
# sudo /etc/init.d/openibd restart &&
wget http://www.mellanox.com/downloads/Drivers/MLNX_DPDK_$DPDK_VERSION.tar.gz &&
tar xfz ./MLNX_DPDK_$DPDK_VERSION.tar.gz &&
cd ./MLNX_DPDK_$DPDK_VERSION &&
make install T=x86_64-native-linuxapp-gcc &&

# Huge pages
# http://dpdk.org/doc/guides-16.04/linux_gsg/sys_reqs.html
sudo sed -i -r 's/GRUB_CMDLINE_LINUX=\"(.*)\"/GRUB_CMDLINE_LINUX=\"\1 default_hugepagesz=1G hugepagesz=1G hugepages=4\"/' /etc/default/grub &&
sudo update-grub &&
sudo umount /mnt &&
sudo rm -rf /mnt/huge_1GB &&
sudo mkdir /mnt/huge_1GB &&
echo 'nodev /mnt/huge_1GB hugetlbfs pagesize=1GB 0 0' | sudo tee -a /etc/fstab &&

# RTE_SDK location
echo "" >> $HOME/.bashrc &&
echo "export RTE_SDK=$HOME/MLNX_DPDK_$DPDK_VERSION" >> $HOME/.bashrc &&
echo "export RTE_TARGET=x86_64-native-linuxapp-gcc" >> $HOME/.bashrc &&
export RTE_SDK=$HOME/MLNX_DPDK_$DPDK_VERSION &&
export RTE_TARGET=x86_64-native-linuxapp-gcc &&

# make Click
cd /etalon/click-etalon &&
./configure --enable-user-multithread --disable-linuxmodule --enable-intel-cpu --enable-nanotimestamp --enable-dpdk &&
make -j `nproc` &&
sudo make -j `nproc` install &&

# make flowgrind
cd /etalon/flowgrind-etalon &&
autoreconf -i &&
./configure &&
make -j `nproc` &&
sudo make -j `nproc` install &&
cp /usr/local/sbin/flowgrindd /etalon/vhost/ &&

# libVT
cd /etalon/libVT &&
sudo make -j `nproc` install &&
sudo cp ./libVT.so /etalon/vhost/ &&

# libADU - Ignoring for now.
# cd /etalon/libADU &&
# sudo make -j `nproc` install &&

# get docker
cd $HOME &&
curl -fsSL get.docker.com -o get-docker.sh &&
sudo sh get-docker.sh &&
rm $HOME/get-docker.sh &&

# PTP
printf '[enp8s0d1]\n' | sudo tee -a /etc/linuxptp/ptp4l.conf &&
sudo sed -i '/(PTP) service/a Requires=network.target\nAfter=network.target' /lib/systemd/system/ptp4l.service &&
sudo sed -i 's/ -i eth0//' /lib/systemd/system/ptp4l.service &&
sudo sed -i 's/-w -s eth0/-c enp8s0d1 -s CLOCK_REALTIME -w/' /lib/systemd/system/phc2sys.service &&
sudo systemctl daemon-reload &&
sudo systemctl enable phc2sys.service &&
sudo systemctl disable ntp.service &&

# vhost SSH
cp /etalon/vhost/config/ssh/id_rsa $HOME/.ssh/ &&
cp /etalon/vhost/config/ssh/id_rsa.pub $HOME/.ssh/ &&
chmod 600 $HOME/.ssh/id_rsa &&
chmod 600 $HOME/.ssh/id_rsa.pub &&

# HiBench
cd $HOME &&
git clone https://github.com/intel-hadoop/HiBench.git &&
cd HiBench/ &&
mvn -Phadoopbench -Dspark=2.1 -Dscala=2.11 clean package &&
rm -rf .git common docker docs flinkbench gearpumpbench hadoopbench sparkbench stormbench travis &&
cd ../ &&
tar cfvz ./HiBench.tar.gz ./HiBench/ &&
rm -rf ./HiBench &&
mv ./HiBench.tar.gz /etalon/vhost/ &&

# protobuff
cd $HOME && \
wget https://github.com/google/protobuf/releases/download/v2.5.0/protobuf-2.5.0.tar.gz &&
tar zxvf protobuf-2.5.0.tar.gz &&
cd protobuf-2.5.0 &&
./configure &&
make -j `nproc` &&
make -j `nproc` check &&
sudo make -j `nproc` install &&
sudo ldconfig &&

# Hadoop 2.9 - We do not need this for now.
# cd $HOME &&
# wget http://apache.mirrors.pair.com/hadoop/common/hadoop-2.9.0/hadoop-2.9.0-src.tar.gz &&
# tar xfvz hadoop-2.9.0-src.tar.gz &&
# cd hadoop-2.9.0-src &&
# cp /etalon/reHDFS/* ./hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/ &&
# mvn package -Pdist,native -DskipTests -Dtar &&
# cp ./hadoop-dist/target/hadoop-2.9.0.tar.gz /etalon/vhost/ &&
# rm -rf $HOME/hadoop-2.9.0-src &&

# Fix broken kill in 16.04
cd $HOME &&
sudo sed -i -e 's/# deb-src/deb-src/' /etc/apt/sources.list &&
sudo apt-get update &&
sudo apt-get source procps &&
sudo apt-get build-dep -y procps &&
cd procps-3.3.10 &&
sudo dpkg-buildpackage &&
cp ./.libs/kill /etalon/vhost/ &&
sudo rm -rf $HOME/libprocps* &&
sudo rm -rf $HOME/procps* &&

if [ $MODE != "--local" ]; then
    # get extra space
    cd $HOME &&
    sudo mkdir /mnt/hdfs &&
    sudo /usr/local/etc/emulab/mkextrafs.pl -f /mnt/hdfs &&
    sudo chown `whoami` /mnt/hdfs &&

    # move docker dir
    sudo service docker stop &&
    sudo mv /var/lib/docker /mnt/hdfs/ &&
    sudo ln -s /mnt/hdfs/docker /var/lib/docker &&
    sudo service docker start
fi &&

echo "done"
# sudo reboot
