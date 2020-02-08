#!/bin/bash
#
# Run NSDI 2020 experiments.

set -o errexit

# Remove old results files that require root priviledges.
sudo rm -fv /tmp/*click.txt
# Remove results from aborted experiments.
rm -fv "$HOME"/1tb/*txt
# Flush the OS buffer cache.
sudo sync;
echo "1" | sudo tee /proc/sys/vm/drop_caches
# Run experiments.
"$HOME"/etalon/experiments/buffers/nsdi2020.py
