#!/bin/bash

# clear arp
sudo ip link set arp off dev enp8s0
sudo ip link set arp on dev enp8s0
sudo ip link set arp off dev enp8s0d1
sudo ip link set arp on dev enp8s0d1
