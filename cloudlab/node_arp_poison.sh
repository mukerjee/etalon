#!/bin/bash

./tune.sh
./clear_arp.sh

for i in {0..7}
do
    ping host$i -c1 -W1
done
ping router -c1 -W1

for i in {0..7}
do
    sudo arp -s host$i `arp | grep router | tr -s ' ' | cut -d' ' -f3`
done
