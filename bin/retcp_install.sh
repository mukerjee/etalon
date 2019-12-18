#!/bin/bash
#
# Install reTCP.
#
# This is done in a cron script on boot because, as part of the install process,
# we install a new kernel and reboot. We want to make sure that the new kernel's
# /lib/modules/$(uname -r) directory has been created before trying to copy the
# reTCP kernel module there.

set -o errexit

cd /etalon/reTCP/
make -j "$(nproc)"
sudo cp -fv retcp.ko "/lib/modules/$(uname -r)"
sudo depmod
sudo modprobe retcp

# Always load the reTCP kernel module on boot.
if ! grep "retcp" /etc/modules; then
    echo "retcp" | sudo tee -a /etc/modules
fi
