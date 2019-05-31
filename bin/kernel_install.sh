#!/bin/bash
#
# Install the Etalon reTCP kernel patch.
#
# The first argument is the mode. "--cl" CloudLab.

set -o errexit

MODE=$1
echo "MODE: $MODE"
BUILD_DIR=$HOME
# UBUNTU_VERSION="bionic"
VER="linux-image-`uname -r`"
SRC_DIR="linux-signed-`echo $VER | cut -d"-" -f3`"

if [ ! -d $HOME/etalon ]; then
    echo "Error: Etalon repo not located at \"$HOME/etalon\"!"
    exit 1
fi
if [ -d $HOME/.config ]; then
    if [ $MODE == "--cl" ]; then
        sudo chown -R `whoami`:dna-PG0 $HOME/.config
    else
        sudo chown -R `whoami`:`whoami` $HOME/.config
    fi
fi

if [ $MODE == "--cl" ]; then
    # Mount a disk on which to build the kernel (the root device does not have
    # sufficient space).
    sudo mkfs.ext4 /dev/sda4
    mkdir $BUILD_DIR/mnt
    sudo mount /dev/sda4 $BUILD_DIR/mnt
    sudo chown -R `whoami`:dna-PG0 $BUILD_DIR/mnt
    BUILD_DIR=$BUILD_DIR/mnt
fi

# Prepare the build directory.
rm -rf $BUILD_DIR/build
mkdir $BUILD_DIR/build
BUILD_DIR=$BUILD_DIR/build
cd $BUILD_DIR

# Install dependencies and download sources.
sudo sed -i "/^# deb-src /s/^# //" /etc/apt/sources.list
sudo apt update
sudo apt -y build-dep $VER
sudo apt install -y \
     git fakeroot libssl-dev libelf-dev libudev-dev libpci-dev flex bison \
     python libiberty-dev libdw-dev elfutils systemtap-sdt-dev libunwind-dev \
     libaudit-dev liblzma-dev libnuma-dev linux-cloud-tools-common \
     linux-tools-common kernel-wedge
apt source $VER
# git clone git://kernel.ubuntu.com/ubuntu/ubuntu-$UBUNTU_VERSION.git $BUILD_DIR/ubuntu-$UBUNTU_VERSION

cd $SRD_DIR
git apply $HOME/etalon/reTCP/kernel-patch.patch
fakeroot debian/rules clean
# Perf needs $PYTHON to be set.
MAKEFLAGS="-j `nproc`" PYTHON=`which python` fakeroot debian/rules binary-headers binary-generic binary-perarch
# make bindev -package
sudo dpkg --force-all -i $BUILD_DIR/*.deb

echo "Done"
# sudo reboot
