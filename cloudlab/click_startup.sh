#!/bin/bash

# run click
# n = 4? n = 3?
# http://dpdk-guide.gitlab.io/dpdk-guide/tuning/memory.html
# http://dpdk.org/doc/guides/nics/mlx4.html
~/sdrt/click-util/gen-switch.py > /tmp/hybrid-switch.click
sudo nice -n -20 ./userlevel/click --dpdk -c 0xFFFF -n 4 --  /tmp/hybrid-switch.click
