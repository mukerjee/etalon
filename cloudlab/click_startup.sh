#!/bin/bash

# run click
# n = 4? n = 3?
# http://dpdk-guide.gitlab.io/dpdk-guide/tuning/memory.html
# http://dpdk.org/doc/guides/nics/mlx4.html
cd $HOME/sdrt/click/
make -j16 && sudo nice -n -20 ./userlevel/click --dpdk -c 0xFFFF -n 4 --  ../click-utils/hybrid-switch.click
