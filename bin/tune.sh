#!/bin/bash
#
# Perform network tuning.

set -o errexit

source /etalon/etc/script_config.sh

# NIC tuning. Data interface.
sudo ethtool -C $DATA_IF tx-usecs 0 | true
sudo ethtool -L $DATA_IF rx 1 | true
sudo service irqbalance stop
sudo /usr/sbin/set_irq_affinity.sh $DATA_IF
# Control interface.
sudo ethtool -C $CONTROL_IF tx-usecs 0 rx-usecs 0 adaptive-rx off | true
sudo service irqbalance stop
sudo /usr/sbin/set_irq_affinity.sh $CONTROL_IF

# Kernel tuning.
sudo sysctl -w net.ipv4.neigh.default.gc_thresh3=8192
sudo sysctl -w net.ipv4.tcp_congestion_control=reno

ulimit -n 4096
