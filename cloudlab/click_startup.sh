#!/bin/bash


$HOME/sdrt/cloudlab/tune.sh

sudo sed -i -r 's/10.10.1.([[:digit:]][[:digit:]])/10.10.2.\1/' /etc/hosts

# run click
# n = 4? n = 3?
# http://dpdk-guide.gitlab.io/dpdk-guide/tuning/memory.html
# http://dpdk.org/doc/guides/nics/mlx4.html
cd $HOME/sdrt/click/
make -j16 && sudo nice -n -20 ./userlevel/click --dpdk -c 0xFFFF -n 4 --  ./conf/dpdk-hybrid-switch.click
