#!/bin/bash
#
# The first argument is the mode. "--local" implies a local cluster. Empty
# implies CloudLab.

set -o errexit

MODE=$1
echo "MODE: $MODE"
BUILD_DIR=$HOME

sudo apt update
sudo apt install -y git tmux htop emacs fakeroot

# Install updated kernel.
sudo sed -i '/^# deb-src /s/^#//' /etc/apt/sources.list
sudo apt update
sudo apt -y build-dep linux-image-$(uname -r)
sudo apt install linux-cloud-tools-common linux-tools-common kernel-wedge

if [ $MODE == "--local" ]; then
    sudo chown -R `whoami`:`whoami` $HOME/.config
else
    # We are running on CloudLab.
    sudo chown -R `whoami`:dna-PG0 $HOME/.config
    # Mount a disk on which to build the kernel (the root device does not have
    # sufficient space).
    sudo mkfs.ext4 /dev/sda4
    mkdir $BUILD_DIR/mnt
    sudo mount /dev/sda4 $BUILD_DIR/mnt
    sudo chown -R `whoami`:dna-PG0 $BUILD_DIR/mnt
    BUILD_DIR=$BUILD_DIR/mnt
fi

# Apply the kernel patch, and compile.
git clone git://kernel.ubuntu.com/ubuntu/ubuntu-xenial.git $BUILD_DIR/ubuntu-xenial
cd $BUILD_DIR/ubuntu-xenial
git apply $BUILD_DIR/etalon/reTCP/kernel-patch.patch
fakeroot debian/rules clean
MAKEFLAGS="-j 16" fakeroot debian/rules binary-headers binary-generic binary-perarch
sudo dpkg -i $BUILD_DIR/*.deb

# Clean up.
cd $HOME
rm -rf $BUILD_DIR/ubuntu-xenial

echo "done"
# sudo reboot
