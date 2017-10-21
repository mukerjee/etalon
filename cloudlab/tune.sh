#!/bin/bash

NUM_HOSTS=8

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
sudo ifconfig $DATA_IF 10.10.$DATA_NET.`echo ${h:4}`/24 mtu 9000
sudo ifconfig $CONTROL_IF 10.10.$CONTROL_NET.`echo ${h:4}`/24

sudo ethtool -C $DATA_IF tx-usecs 0
sudo ethtool -L $DATA_IF rx 1
sudo service irqbalance stop
sudo /usr/sbin/set_irq_affinity.sh $DATA_IF

sudo ethtool -C $CONTROL_IF tx-usecs 0 rx-usecs 0 adaptive-rx off
sudo service irqbalance stop
sudo /usr/sbin/set_irq_affinity.sh $CONTROL_IF

# sudo sysctl -w net.ipv4.tcp_wmem="98304 98304 98304"

for i in `seq 1 $NUM_HOSTS`
do
    for j in `seq 1 $NUM_HOSTS`
    do
	    if ! grep -q "h$i$j" /etc/hosts
	    then
	        printf "%s\t%s\n" "10.10.$DATA_NET.$i$j" "h$i$j" | sudo tee -a /etc/hosts
	    fi
    done
done

if hostname | grep -q switch
then  # switch
    sudo sed -i "s/10.10.$DATA_NET/10.10.$CONTROL_NET/" /etc/hosts
else  # host
    sudo sed -i "s/10.10.$CONTROL_NET/10.10.$DATA_NET/" /etc/hosts
fi

if ! hostname | grep -q switch
then
    ~/sdrt/cloudlab/arp_clear.sh
    ~/sdrt/cloudlab/arp_poison.sh
fi
