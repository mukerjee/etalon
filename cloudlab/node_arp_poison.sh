#!/bin/bash

h=`hostname | cut -d'.' -f1`
sudo ifconfig eth2 10.10.1.`echo ${h:4} | awk '{print $1+2}'` netmask 255.255.255.0
sudo ifconfig eth3 10.10.2.`echo ${h:4} | awk '{print $1+2}'` netmask 255.255.255.0

sudo ifconfig eth2 mtu 9000
sudo ifconfig eth2 txqueuelen 10000

sudo ethtool -A eth2 rx on tx on
sudo ethtool -A eth3 rx off tx off

sudo ethtool -L eth2 rx 1 tx 1
sudo ethtool -L eth3 rx 1 tx 1
sudo ethtool -C eth2 adaptive-rx off
sudo ethtool -C eth3 adaptive-rx off

sudo sysctl -w net.core.netdev_max_backlog=250000
sudo sysctl -w net.core.optmem_max=16777216
sudo sysctl -w net.core.rmem_default=16777216
sudo sysctl -w net.core.wmem_default=16777216
sudo sysctl -w net.core.rmem_max=268435456
sudo sysctl -w net.core.wmem_max=268435456

sudo sysctl -w net.ipv4.tcp_rmem="4096 87380 134217728"
sudo sysctl -w net.ipv4.tcp_wmem="4096 65536 134217728"

# sudo sysctl -w net.ipv4.tcp_timestamps=1
# sudo sysctl -w net.ipv4.tcp_sack=1
# sudo sysctl -w net.ipv4.tcp_low_latency=0
# sudo sysctl -w net.ipv4.tcp_mtu_probing=0
sudo sysctl -w net.ipv4.tcp_adv_win_scale=1
sudo sysctl -w net.ipv4.tcp_congestion_control=westwood


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