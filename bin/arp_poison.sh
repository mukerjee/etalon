#!/bin/bash

source /etalon/etc/script_config.sh

/etalon/bin/arp_clear.sh

sudo ping switch -c1 -W1

for i in $(seq 1 "$NUM_RACKS"); do
    sudo arp -s "host$i" "$(arp | grep switch | tr -s " " | cut -d" " -f3)"
done
