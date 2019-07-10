#!/bin/bash

export NUM_RACKS=3
export HOSTS_PER_RACK=16

# networks
export DATA_IF=enp68s0
export DATA_NET=1
export CONTROL_IF=eno4
export CONTROL_NET=2

# switch
export SWITCH_DATA_IP=10.$DATA_NET.100.100
export SWITCH_CONTROL_IP=10.$CONTROL_NET.100.100

# fqdn e.g., h37.etalon.local
export FQDN=etalon.local
