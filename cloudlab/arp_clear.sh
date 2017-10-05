#!/bin/bash

DATA_IF=enp8s0
CONTROL_IF=enp8s0d1

# clear arp
sudo ip link set arp off dev $DATA_IF
sudo ip link set arp on dev $DATA_IF
sudo ip link set arp off dev $CONTROL_IF
sudo ip link set arp on dev $CONTROL_IF
