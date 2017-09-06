#!/bin/bash

h=`hostname | cut -d'.' -f1`
sudo ifconfig enp8s0 10.10.1.`echo ${h:4} | awk '{print $1+2}'` netmask 255.255.255.0
sudo ifconfig enp8s0d1 10.10.2.`echo ${h:4} | awk '{print $1+2}'` netmask 255.255.255.0

for i in {0..15}
do
    ping host$i -c1 -W1
done
ping router -c1 -W1

for i in {0..15}
do
    sudo arp -s host$i `arp | grep router | tr -s ' ' | cut -d' ' -f3`
done
