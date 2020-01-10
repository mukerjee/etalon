#!/bin/bash

set -o errexit

# Generate hybrid switch definition.
/etalon/bin/gen_switch.py after > /tmp/hybrid-switch.click
# Clear allocated hugepages, in case the previous run crashed and did not
# deallocate them. (Not sure if this is actually necessary.)
sudo rm -rfv /dev/hugepages/* /mnt/huge/*
# Run the hybrid switch.
#
# Setting the coremask: On the Intel E5-2680's in the MAAS cluster, the
# even-numbered cores are on socket 0 and the odd cores are on socket 1. The NIC
# is attached to socket 1. So we need to only use the odd cores.
#
# Setting the memory channels: n = 4? n = 3?
# http://dpdk-guide.gitlab.io/dpdk-guide/tuning/memory.html
# http://dpdk.org/doc/guides/nics/mlx4.html
sudo nice -n -20 \
     click --dpdk -l 1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39 \
     -n 4 -- /tmp/hybrid-switch.click
