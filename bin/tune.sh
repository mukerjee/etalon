#!/bin/bash

source /etalon/etc/script_config.sh

# set IPs
if ! hostname | grep -q switch; then
    h=`hostname | cut -d'.' -f1`
else
    h='host100'
fi
sudo ifconfig $DATA_IF 10.$DATA_NET.100.`echo ${h:4}`/16 mtu 9000
sudo ifconfig $CONTROL_IF 10.$CONTROL_NET.100.`echo ${h:4}`/16

# NIC tuning
sudo ethtool -C $DATA_IF tx-usecs 0
sudo ethtool -L $DATA_IF rx 1
sudo service irqbalance stop
sudo /usr/sbin/set_irq_affinity.sh $DATA_IF

sudo ethtool -C $CONTROL_IF tx-usecs 0 rx-usecs 0 adaptive-rx off
sudo service irqbalance stop
sudo /usr/sbin/set_irq_affinity.sh $CONTROL_IF

# kernel tuning
sudo sysctl -w net.ipv4.neigh.default.gc_thresh3=8192
sudo sysctl -w net.ipv4.tcp_congestion_control=reno

# make our own /etc/hosts
printf "%s\t%s\n" "127.0.0.1" "localhost" | sudo tee /etc/hosts
printf "%s\t%s\n" $SWITCH_DATA_IP "switch" | sudo tee -a /etc/hosts

for i in `seq 1 $NUM_RACKS`; do
    printf "%s\t%s\n" "10.$DATA_NET.100.$i" "host$i" | sudo tee -a /etc/hosts
done

for i in `seq 1 $NUM_RACKS`; do
    for j in `seq 1 $HOSTS_PER_RACK`; do
        printf "%s\t%s\n" "10.$DATA_NET.$i.$j" "h$i$j.$FQDN" | sudo tee -a /etc/hosts
    done
done

if hostname | grep -q switch; then
    # switch
    sudo sed -i "s/10\.$DATA_NET\./10\.$CONTROL_NET\./" /etc/hosts
fi

# build reTCP
cd /etalon/reTCP/
make -j `nproc`
sudo insmod retcp.ko

ulimit -n 4096
