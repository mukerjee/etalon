#!/bin/bash

NUM_RACKS=8
HOSTS_PER_RACK=16

DATA_IF=enp8s0
CONTROL_IF=enp8s0d1
DATA_NET=1
CONTROL_NET=2

if ! hostname | grep -q switch
then
    h=`hostname | cut -d'.' -f1`
else
    h='host9'
fi
sudo ifconfig $DATA_IF 10.$DATA_NET.10.`echo ${h:4}`/16 mtu 9000
sudo ifconfig $CONTROL_IF 10.$CONTROL_NET.10.`echo ${h:4}`/16

sudo ethtool -C $DATA_IF tx-usecs 0
sudo ethtool -L $DATA_IF rx 1
sudo service irqbalance stop
sudo /usr/sbin/set_irq_affinity.sh $DATA_IF

sudo ethtool -C $CONTROL_IF tx-usecs 0 rx-usecs 0 adaptive-rx off
sudo service irqbalance stop
sudo /usr/sbin/set_irq_affinity.sh $CONTROL_IF

sudo sysctl -w net.ipv4.neigh.default.gc_thresh3=2048
sudo sysctl -w net.ipv4.tcp_congestion_control=reno

for i in `seq 1 $NUM_RACKS`
do
    for j in `seq 1 $HOSTS_PER_RACK`
    do
	    if ! grep -q "h$i$j" /etc/hosts
	    then
	        printf "%s\t%s\n" "10.$DATA_NET.$i.$j" "h$i$j" | sudo tee -a /etc/hosts
	    fi
    done
done

sudo sed -i "s/10\.10\.$DATA_NET\./10\.$DATA_NET\.10\./" /etc/hosts
sudo sed -i "s/10\.10\.$CONTROL_NET\./10\.$CONTROL_NET\.10\./" /etc/hosts

if hostname | grep -q switch
then  # switch
    sudo sed -i "s/10\.$DATA_NET\./10\.$CONTROL_NET\./" /etc/hosts
else  # host
    sudo sed -i "s/10\.$CONTROL_NET\./10\.$DATA_NET\./" /etc/hosts
fi

if ! hostname | grep -q switch
then
    $HOME/sdrt/cloudlab/arp_clear.sh
    $HOME/sdrt/cloudlab/arp_poison.sh
fi
