#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root"
    exit 1
fi

OTHER_USER=`who | head -n1 | cut -f1 -d' '`

sudo killall -9 iperf3 iperf nuttcp mn 2> /dev/null
ps aux | grep sshd | grep -v grep | grep -v $OTHER_USER | awk {'print $2'} | xargs sudo kill 2> /dev/null
sudo killall -9 $(pgrep mininet_startup | grep -v $$) 2> /dev/null
sudo tc qdisc del dev eth2 root 2> /dev/null
sudo mn -c &> /dev/null
