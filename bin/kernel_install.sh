#!/bin/bash
#
# The first argument is the mode. "--local" implies a local cluster. Empty
# implies CloudLab.

set -o errexit

MODE=$1
echo "MODE: $MODE"
BUILD_DIR=$HOME
UBUNTU_VERSION="bionic"

sudo apt update
sudo apt install -y git tmux htop emacs fakeroot libssl-dev libelf-dev

# Install updated kernel.
sudo sed -i '/^# deb-src /s/^#//' /etc/apt/sources.list
sudo apt update
sudo apt -y build-dep linux-image-$(uname -r)
sudo apt install linux-cloud-tools-common linux-tools-common kernel-wedge

if [ $MODE == "--local" ]; then
    if [ -d $HOME/.config ]; then
        sudo chown -R `whoami`:`whoami` $HOME/.config
    fi
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
git clone git://kernel.ubuntu.com/ubuntu/ubuntu-$UBUNTU_VERSION.git $BUILD_DIR/ubuntu-$UBUNTU_VERSION
cd $BUILD_DIR/ubuntu-$UBUNTU_VERSION
git apply $BUILD_DIR/etalon/reTCP/kernel-patch.patch
fakeroot debian/rules clean
# For some unknown reason, this command fails with a return code of 2 but does
# not print any error messages. Strangley, running it again solves the problem.
MAKEFLAGS="-j `nproc`" fakeroot debian/rules binary-headers binary-generic binary-perarch || \
    MAKEFLAGS="-j `nproc`" fakeroot debian/rules binary-headers binary-generic binary-perarch
sudo dpkg -i $BUILD_DIR/*.deb

# Clean up.
cd
rm -rf $BUILD_DIR/ubuntu-$UBUNTU_VERSION

echo "Done"
# sudo reboot
