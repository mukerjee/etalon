#!/bin/bash

sudo ifconfig eth2 10.10.1.1 netmask 255.255.255.0
sudo ifconfig eth3 10.10.2.1 netmask 255.255.255.0

sudo sed -i -r 's/10.10.2/10.10.1/' /etc/hosts

# run click
# n = 4? n = 3?
# http://dpdk-guide.gitlab.io/dpdk-guide/tuning/memory.html
# http://dpdk.org/doc/guides/nics/mlx4.html
cd $HOME/sdrt/click/
sudo nice -n -20 ./userlevel/click --dpdk -c 0xFF -n 4 -- -j 8 ./conf/dpdk-hybrid-switch.click
