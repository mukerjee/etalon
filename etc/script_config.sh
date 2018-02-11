#!/bin/bash

export NUM_RACKS=8
export HOSTS_PER_RACK=16

export DATA_IF=enp8s0
export CONTROL_IF=enp8s0d1

export DATA_NET=1
export CONTROL_NET=2

export SWITCH_DATA_IP=10.$DATA_NET.100.100

export FQDN=etalon.local
