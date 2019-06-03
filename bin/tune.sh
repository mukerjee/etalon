#!/bin/bash
#
# First argument is the hostname.

source /etalon/etc/script_config.sh

NEW_HOSTNAME=$1
if [ -z $NEW_HOSTNAME ]; then
    echo "Error: Must provide hostname!"
    exit 1
fi

# Change the hostname.
OLD_HOSTNAME=`hostname`
sudo hostname $NEW_HOSTNAME
sed -i "s/$OLD_HOSTNAME/$NEW_HOSTNAME/g" /etc/hostname
sed -i "s/$OLD_HOSTNAME/$NEW_HOSTNAME/g" /etc/hosts

# Set IPs.
# sudo ifconfig $DATA_IF 10.$DATA_NET.100.`echo ${h:4}`/16 mtu 9000
# sudo ifconfig $CONTROL_IF 10.$CONTROL_NET.100.`echo ${h:4}`/16
if hostname | grep -q switch; then
    sudo ip addr add $SWITCH_DATA_IP/24 dev $DATA_IF
    sudo ip addr add $SWITCH_CONTROL_IP/24 dev $CONTROL_IF
else
    H=`hostname | cut -d'.' -f1`
    sudo ip addr add 10.$DATA_NET.100.`echo ${H:4}`/24 dev $DATA_IF
    sudo ip addr add 10.$CONTROL_NET.100.`echo ${H:4}`/24 dev $CONTROL_IF
fi

# NIC tuning. Data interface.
sudo ethtool -C $DATA_IF tx-usecs 0
sudo ethtool -L $DATA_IF rx 1
sudo service irqbalance stop
sudo /usr/sbin/set_irq_affinity.sh $DATA_IF
# Control interface.
sudo ethtool -C $CONTROL_IF tx-usecs 0 rx-usecs 0 adaptive-rx off
sudo service irqbalance stop
sudo /usr/sbin/set_irq_affinity.sh $CONTROL_IF

# Kernel tuning.
sudo sysctl -w net.ipv4.neigh.default.gc_thresh3=8192
sudo sysctl -w net.ipv4.tcp_congestion_control=reno

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
if hostname | grep -q switch; then
    # If this is the switch, then communicate with everyone over the control
    # network instead of the data network.
    sudo sed -i "s/10\.$DATA_NET\./10\.$CONTROL_NET\./g" /etc/hosts
fi

# build reTCP - Disable for now.
# cd /etalon/reTCP/
# make -j `nproc`
# sudo insmod retcp.ko

ulimit -n 4096
