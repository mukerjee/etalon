#!/bin/bash
#
# The first argument is the mode. "--local" implies a local cluster. Empty
# implies CloudLab.

set -o errexit

MODE=$1
echo "MODE: $MODE"
OFED_VERSION=4.1-1.0.2.0

if [ ! -d ~/etalon ]; then
    echo "Error: Etalon repo not located at \"~/etalon\"!"
    exit 1
fi

sudo apt-get update && sudo apt-get install -y \
                            git \
                            python-pip \
                            linuxptp &&

sudo pip install rpyc &&

# get Etalon - Assume we downloaded it manually.
# cd $HOME &&
# GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no" git clone --recursive https://github.com/ccanel/etalon.git &&

cd / &&
sudo ln -sf ~/etalon &&

(crontab -l 2>/dev/null; echo "@reboot sleep 60 && $HOME/etalon/bin/tune.sh") | crontab - &&
sudo rm -f /var/run/crond.reboot &&


# Mellanox OFED - Assume that this has been installed manually for now.
# http://www.mellanox.com/related-docs/prod_software/Mellanox_OFED_Linux_User_Manual_v4.0.pdf
# cd $HOME &&
# wget http://www.mellanox.com/downloads/ofed/MLNX_OFED-$OFED_VERSION/MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz &&
# tar xfz ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64.tgz &&
# sudo ./MLNX_OFED_LINUX-$OFED_VERSION-ubuntu16.04-x86_64/mlnxofedinstall --force &&
sudo connectx_port_config -c eth,eth &&
# sudo /etc/init.d/openibd restart &&

# get docker
cd $HOME &&
curl -fsSL get.docker.com -o get-docker.sh &&
sudo sh get-docker.sh &&

# get pipework
cd $HOME &&
sudo bash -c "curl https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework > /usr/local/bin/pipework" &&
sudo chmod +x /usr/local/bin/pipework &&

sudo systemctl enable /etalon/rpycd/rpycd.service &&

# PTP
printf 'slaveOnly\t\t1\n[enp68s0]\n' | sudo tee -a /etc/linuxptp/ptp4l.conf &&
sudo sed -i '/(PTP) service/a Requires=network.target\nAfter=network.target' /lib/systemd/system/ptp4l.service &&
sudo sed -i 's/ -i eth0//' /lib/systemd/system/ptp4l.service &&
sudo sed -i 's/-w -s eth0/-a -r/' /lib/systemd/system/phc2sys.service &&
sudo systemctl daemon-reload &&
sudo systemctl enable phc2sys.service &&
sudo systemctl disable ntp.service &&

if [ $MODE != "--local" ]; then
    # get extra space
    sudo mkdir /mnt/hdfs &&
    sudo /usr/local/etc/emulab/mkextrafs.pl -f /mnt/hdfs &&
    sudo chown `whoami` /mnt/hdfs &&

    # move docker dir
    sudo service docker stop &&
    sudo mv /var/lib/docker /mnt/hdfs/ &&
    sudo ln -s /mnt/hdfs/docker /var/lib/docker &&
    sudo service docker start
fi &&

echo "Done"
# sudo reboot
