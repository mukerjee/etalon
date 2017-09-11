#!/bin/bash

sudo ifconfig eth2 10.10.1.1 netmask 255.255.255.0
sudo ifconfig eth3 10.10.2.1 netmask 255.255.255.0

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

# sudo sysctl -w net.ipv4.tcp_mem="374847 499797 749694"
# sudo sysctl -w net.ipv4.udp_mem="374847 499797 749694"
sudo sysctl -w net.ipv4.tcp_rmem="4096 87380 134217728"
sudo sysctl -w net.ipv4.tcp_wmem="4096 65536 134217728"

sudo sysctl -w net.ipv4.tcp_timestamps=1
# sudo sysctl -w net.ipv4.tcp_sack=1
sudo sysctl -w net.ipv4.tcp_low_latency=0
sudo sysctl -w net.ipv4.tcp_mtu_probing=1
sudo sysctl -w net.ipv4.tcp_adv_win_scale=1
sudo sysctl -w net.ipv4.tcp_congestion_control=westwood


sudo sed -i -r 's/10.10.2/10.10.1/' /etc/hosts

# run click
# n = 4? n = 3?
# http://dpdk-guide.gitlab.io/dpdk-guide/tuning/memory.html
# http://dpdk.org/doc/guides/nics/mlx4.html
cd $HOME/sdrt/click/
sudo nice -n -20 ./userlevel/click --dpdk -c 0xFF -n 4 -- -j 16 ./conf/dpdk-hybrid-switch.click
# sudo nice -n -20 ./userlevel/click --dpdk -c 0xFF -n 4 -- -j 16 ./conf/dpdk-circuit.click
