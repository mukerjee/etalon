#!/bin/bash

sudo killall -9 iperf3 iperf nuttcp mn mininet_startup.py
sudo tc qdisc del dev eth2 root
sudo mn -c
