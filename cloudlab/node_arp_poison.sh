#!/bin/bash

h=`hostname | cut -d'.' -f1`
sudo ifconfig eth2 10.10.1.`echo ${h:4} | awk '{print $1+2}'` netmask 255.255.255.0
sudo ifconfig eth3 10.10.2.`echo ${h:4} | awk '{print $1+2}'` netmask 255.255.255.0

sudo sed -i -r 's/10.10.2/10.10.1/' /etc/hosts

# clear arp
sudo ip link set arp off dev eth2
sudo ip link set arp on dev eth2
sudo ip link set arp off dev eth3
sudo ip link set arp on dev eth3

for i in {0..7}
do
    ping host$i -c1 -W1
done
ping router -c1 -W1

for i in {0..7}
do
    sudo arp -s host$i `arp | grep router | tr -s ' ' | cut -d' ' -f3`
done
