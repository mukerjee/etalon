#!/bin/bash

NUM_HOSTS=8

$HOME/sdrt/cloudlab/arp_clear.sh

sudo ping switch -c1 -W1

for i in `seq 1 $NUM_HOSTS`
do
    sudo arp -s host$i `arp | grep switch | tr -s ' ' | cut -d' ' -f3`
done
