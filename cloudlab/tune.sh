#!/bin/bash

DATA_IF=enp8s0
CONTROL_IF=enp8s0d1

if ! hostname | grep -q switch
then
    h=`hostname | cut -d'.' -f1`
else
    h='host9'
fi
sudo ifconfig $DATA_IF 10.10.1.`echo ${h:4}'`/24 mtu 9000
sudo ifconfig $CONTROL_IF 10.10.2.`echo ${h:4}'`/24

sudo ethtool -C $CONTROL_IF tx-usecs 0
sudo ethtool -L $DATA_IF rx 1
sudo service irqbalance stop
sudo /usr/sbin/set_irq_affinity.sh $DATA_IF

sudo sed -i -r 's/10.10.2/10.10.1/' /etc/hosts

for i in {1..8}
do
    for j in {1..8}
    do
	if ! grep -q "h$i$j" /etc/hosts
	then
	    printf "%s\t%s\n" "10.10.1.$i$j" "h$i$j" | sudo tee -a /etc/hosts
	fi
    done
done

if hostname | grep -q switch
then
    sudo sed -i -r 's/10.10.1.([[:digit:]][[:digit:]])/10.10.2.\1/' /etc/hosts
fi
