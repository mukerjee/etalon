#!/bin/bash
#
# Install the Etalon reTCP kernel patch.

set -o errexit

UBUNTU_VERSION="bionic"
BUILD_DIR=$HOME/build
SRC_DIR=$BUILD_DIR/ubuntu-$UBUNTU_VERSION

# Validate.
if [ ! -d $HOME/etalon ]; then
    echo "Error: Etalon repo not located at \"$HOME/etalon\"!"
    exit 1
fi

# Prepare the build directory.
rm -rf $BUILD_DIR
mkdir -pv $BUILD_DIR

# Install dependencies and download sources.
sudo sed -i "/^# deb-src /s/^# //" /etc/apt/sources.list
sudo apt update
sudo apt -y build-dep linux-image-`uname -r`
sudo apt install -y \
    git fakeroot libssl-dev libelf-dev libudev-dev libpci-dev flex bison \
    python libiberty-dev libdw-dev elfutils systemtap-sdt-dev libunwind-dev \
    libaudit-dev liblzma-dev libnuma-dev linux-cloud-tools-common \
    linux-tools-common kernel-wedge
git clone git://kernel.ubuntu.com/ubuntu/ubuntu-$UBUNTU_VERSION.git $SRC_DIR

# Apply the patch, compile and install.
cd $SRC_DIR
git checkout Ubuntu-4.15.0-50.54
git apply $HOME/etalon/reTCP/kernel-patch.patch
fakeroot debian/rules clean
# Perf needs $PYTHON to be set.
MAKEFLAGS="-j `nproc`" PYTHON=`which python` fakeroot debian/rules \
         binary-headers binary-generic binary-perarch
sudo dpkg --force-all -i $BUILD_DIR/*.deb
echo "\nGRUB_DEFAULT=\"Advanced options for Ubuntu>Ubuntu, with Linux 4.15.0-50-generic\"" \
    | sudo tee -a /etc/default/grub
sudo update-grub
