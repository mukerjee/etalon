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
sudo apt install -y git fakeroot libssl-dev libelf-dev libudev-dev libpci-dev

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
# For some unknown reason, the compile command fails with an exit code of 2 but
# does not print any error messages. Running it again solves the problem, but
# also returns an erroneous exit code. Bash order of operations ensures that, if
# this is line n, then if line n + 1 fails, then line n + 2 will be executed.
# Line n + 3 ensures that lines n + 1 and n + 2 always return an exit code of 0.
# In other words, the program will only stop if there is an error on line n + 4.
MAKEFLAGS="-j `nproc`" fakeroot debian/rules binary-headers binary-generic binary-perarch ||
MAKEFLAGS="-j `nproc`" fakeroot debian/rules binary-headers binary-generic binary-perarch ||
true
sudo dpkg -i $BUILD_DIR/*.deb

# Clean up.
cd
rm -rf $BUILD_DIR/ubuntu-$UBUNTU_VERSION

echo "Done"
# sudo reboot
