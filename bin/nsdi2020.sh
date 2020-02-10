#!/bin/bash
#
# Run NSDI 2020 experiments.

set -o errexit

function cleanup {
    # Remove old results files that require root priviledges.
    sudo rm -fv /tmp/*click.txt
    # Remove results from aborted experiments.
    rm -fv "$HOME"/1tb/*txt
}

rm -fv /tmp/docker_built

cleanup
# Flush the OS buffer cache.
sudo sync;
echo "1" | sudo tee /proc/sys/vm/drop_caches
# Run experiments.
"$HOME"/etalon/experiments/buffers/nsdi2020.py
cleanup

ssh -t host1 'sudo sysctl -w net.ipv4.tcp_congestion_control=cubic'
ssh -t host2 'sudo sysctl -w net.ipv4.tcp_congestion_control=cubic'
ssh -t host3 'sudo sysctl -w net.ipv4.tcp_congestion_control=cubic'
