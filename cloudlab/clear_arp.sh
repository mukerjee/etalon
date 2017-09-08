#!/bin/bash

# clear arp
sudo ip link set arp off dev eth2
sleep 1; sudo ip link set arp on dev eth2
