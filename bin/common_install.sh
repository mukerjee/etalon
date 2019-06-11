#!/bin/bash
#
# Install software common to both the Etalon nodes and switch.
#
# The first argument is the new hostname.

set -o errexit

NEW_HOSTNAME=$1
UBUNTU_VERSION=18.04
OFED_VERSION=4.6-1.0.1.1

# Validate.
if [ -z $NEW_HOSTNAME ]; then
    echo "Error: Must provide a hostname!"
    exit 1
fi
if [ ! -d $HOME/etalon ]; then
    echo "Error: Etalon repo not located at \"$HOME/etalon\"!"
    exit 1
fi

# Put etalon in a global location.
sudo rm -rfv /etalon
sudo ln -sfv $HOME/etalon /etalon

# Change the hostname.
OLD_HOSTNAME=`hostname`
sudo hostname $NEW_HOSTNAME
sudo sed -i "s/$OLD_HOSTNAME/$NEW_HOSTNAME/g" /etc/hostname
sudo sed -i "s/$OLD_HOSTNAME/$NEW_HOSTNAME/g" /etc/hosts
if ! [ `hostname` == $NEW_HOSTNAME ]; then
    echo "Error: Problem setting hostname to $NEW_HOSTNAME"
    exit 1
fi

# Set IPs.
source /etalon/etc/script_config.sh
if echo $NEW_HOSTNAME | grep -q switch; then
    NEW_CONTROL_IP=$SWITCH_CONTROL_IP
    NEW_DATA_IP=$SWITCH_DATA_IP
else
    HOST_NUM=`echo ${NEW_HOSTNAME:4}`
    NEW_CONTROL_IP=10.$CONTROL_NET.100.$HOST_NUM
    NEW_DATA_IP=10.$DATA_NET.100.$HOST_NUM
fi
sudo cp /etalon/etc/netplan/01-etalon.yaml /etc/netplan/01-etalon.yaml
sudo sed -i "s/CONTROL_IP/$NEW_CONTROL_IP/g" /etc/netplan/01-etalon.yaml
sudo sed -i "s/DATA_IP/$NEW_DATA_IP/g" /etc/netplan/01-etalon.yaml
sudo netplan apply

# Make our own /etc/hosts.
printf "%s\t%s\n" "127.0.0.1" "localhost" | sudo tee /etc/hosts
printf "%s\t%s\n" $SWITCH_DATA_IP "switch" | sudo tee -a /etc/hosts
# Add all the physical hosts to /etc/hosts.
for i in `seq 1 $NUM_RACKS`; do
    printf "%s\t%s\n" "10.$DATA_NET.100.$i" "host$i" | sudo tee -a /etc/hosts
done
# Add all the emulated hosts to /etc/hosts.
for i in `seq 1 $NUM_RACKS`; do
    for j in `seq 1 $HOSTS_PER_RACK`; do
        printf "%s\t%s\n" "10.$DATA_NET.$i.$j" "h$i$j.$FQDN" | sudo tee -a /etc/hosts
    done
done
if echo $NEW_HOSTNAME | grep -q switch; then
    # If this is the switch, then communicate with everyone over the control
    # network instead of the data network.
    sudo sed -i "s/10\.$DATA_NET\./10\.$CONTROL_NET\./g" /etc/hosts
fi

# Fix permissions of ~/.config.
if [ -d $HOME/.config ]; then
    sudo chown -R `whoami`:`whoami` $HOME/.config
fi

# Install dependencies.
sudo apt update
sudo apt install -y git linuxptp python-pip
sudo -H pip install numpy rpyc
sudo systemctl enable /etalon/rpycd/rpycd.service

# Install Mellanox OFED.
# https://docs.mellanox.com/display/MLNXOFEDv461000/Introduction
echo "Installing MLNX OFED..."
# Download and mount the ISO.
cd
wget http://www.mellanox.com/downloads/ofed/MLNX_OFED-$OFED_VERSION/MLNX_OFED_LINUX-$OFED_VERSION-ubuntu$UBUNTU_VERSION-x86_64.iso
if mount | grep "/mnt "; then
    sudo umount /mnt
fi
sudo mount -o ro,loop MLNX_OFED_LINUX-$OFED_VERSION-ubuntu$UBUNTU_VERSION-x86_64.iso /mnt
# Install.
sudo /mnt/mlnxofedinstall --kernel-only --force
sudo connectx_port_config -c eth,eth
echo 'options mlx4_core log_num_mgm_entry_size=-7' | sudo tee -a /etc/modprobe.d/mlx4.conf
sudo /etc/init.d/openibd restart
# Clean up, but keep the ISO as a record.
sudo umount /mnt

# Install Docker.
echo "Installing docker..."
curl -fsSL https://get.docker.com -o $HOME/get-docker.sh
sudo sh $HOME/get-docker.sh
rm -fv $HOME/get-docker.sh

# Install PTP.
echo "Setting up PTP..."
printf '[enp68s0]\n' | sudo tee -a /etc/linuxptp/ptp4l.conf
sudo sed -i '/(PTP) service/a Requires=network.target\nAfter=network.target' /lib/systemd/system/ptp4l.service
sudo sed -i 's/ -i eth0//' /lib/systemd/system/ptp4l.service
sudo sed -i 's/-w -s eth0/-c enp68s0 -s CLOCK_REALTIME -w/' /lib/systemd/system/phc2sys.service
sudo systemctl daemon-reload
sudo systemctl enable phc2sys.service
if systemctl list-unit-files | grep ntp.service; then
    sudo systemctl disable ntp.service
fi
