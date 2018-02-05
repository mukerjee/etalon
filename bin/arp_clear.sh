#!/bin/bash

source /etalon/etc/script_config.sh

# clear arp
sudo ip link set arp off dev $DATA_IF
sudo ip link set arp on dev $DATA_IF
sudo ip link set arp off dev $CONTROL_IF
sudo ip link set arp on dev $CONTROL_IF
