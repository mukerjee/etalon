#!/bin/bash

h=`hostname | cut -d'.' -f1`
sudo ifconfig eth2 10.10.1.`echo ${h:4} | awk '{print $1+2}'` netmask 255.255.255.0
sudo ifconfig eth3 10.10.2.`echo ${h:4} | awk '{print $1+2}'` netmask 255.255.255.0

sudo sed -i -r 's/10.10.2/10.10.1/' /etc/hosts

sudo ip -s neigh flush all  # clear arp cache

for i in {0..7}
do
    ping host$i -c1 -W1
done
ping router -c1 -W1

for i in {0..7}
do
    sudo arp -s host$i `arp | grep router | tr -s ' ' | cut -d' ' -f3`
done
