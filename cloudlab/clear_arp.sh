#!/bin/bash

# clear arp
sudo ip link set arp off dev eth2
sudo ip link set arp on dev eth2
sudo ip link set arp off dev eth3
sudo ip link set arp on dev eth3
