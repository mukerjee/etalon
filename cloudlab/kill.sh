#!/bin/bash

sudo killall -9 iperf3 iperf nuttcp mn
sudo killall -9 $(pgrep mininet_startup | grep -v $$)
sudo tc qdisc del dev eth2 root
sudo mn -c
