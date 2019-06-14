#!/bin/bash

/etalon/bin/gen-switch.py > /tmp/hybrid-switch.click
# Clear allocated hugepages, in case the previous run crashed and did not
# deallocate them. Not sure if this is actually necessary, but just in case...
sudo rm -rfv /dev/hugepages/* /mnt/huge/*
# Run the hybrid router.
# Memory channels... n = 4? n = 3?
# http://dpdk-guide.gitlab.io/dpdk-guide/tuning/memory.html
# http://dpdk.org/doc/guides/nics/mlx4.html
sudo nice -n -20 click --dpdk -l 0-39 -n 4 --  /tmp/hybrid-switch.click
