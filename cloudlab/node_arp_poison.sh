#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root"
    exit 1
fi

$HOME/sdrt/cloudlab/tune.sh
$HOME/sdrt/cloudlab/clear_arp.sh

# for i in {0..7}
# do
#     sudo ping host$i -c1 -W1
# done
sudo ping router -c1 -W1

for i in {0..7}
do
    sudo arp -s host$i `arp | grep router | tr -s ' ' | cut -d' ' -f3`
done
