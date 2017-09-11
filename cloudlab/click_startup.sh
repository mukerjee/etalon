#!/bin/bash

./tune.sh

# run click
# n = 4? n = 3?
# http://dpdk-guide.gitlab.io/dpdk-guide/tuning/memory.html
# http://dpdk.org/doc/guides/nics/mlx4.html
cd $HOME/sdrt/click/
#sudo nice -n -20 ./userlevel/click --dpdk -c 0xFF -n 4 -- -j 16 ./conf/dpdk-hybrid-switch.click
sudo nice -n -20 ./userlevel/click --dpdk -c 0xFF -n 4 -- -j 16 ./conf/dpdk-circuit.click
