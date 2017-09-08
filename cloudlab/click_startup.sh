#!/bin/bash

sudo ifconfig eth2 10.10.1.1 netmask 255.255.255.0
sudo ifconfig eth3 10.10.2.1 netmask 255.255.255.0

sudo sysctl -w net.ipv4.tcp_timestamps=0
sudo sysctl -w net.ipv4.tcp_sack=1
sudo sysctl -w net.core.netdev_max_backlog=250000
sudo sysctl -w net.core.rmem_max=4194304
sudo sysctl -w net.core.wmem_max=4194304
sudo sysctl -w net.core.rmem_default=4194304
sudo sysctl -w net.core.wmem_default=4194304
sudo sysctl -w net.core.optmem_max=4194304
sudo sysctl -w net.ipv4.tcp_rmem="4096 87380 4194304"
sudo sysctl -w net.ipv4.tcp_wmem="4096 65536 4194304"
sudo sysctl -w net.ipv4.tcp_low_latency=1
sudo sysctl -w net.ipv4.tcp_adv_win_scale=1
sudo ifconfig eth2 mtu 9000

sudo sed -i -r 's/10.10.2/10.10.1/' /etc/hosts

# run click
# n = 4? n = 3?
# http://dpdk-guide.gitlab.io/dpdk-guide/tuning/memory.html
# http://dpdk.org/doc/guides/nics/mlx4.html
cd $HOME/sdrt/click/
sudo nice -n -20 ./userlevel/click --dpdk -c 0xFF -n 4 -- -j 8 ./conf/dpdk-hybrid-switch.click
