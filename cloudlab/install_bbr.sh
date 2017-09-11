#!/bin/bash

cd $HOME

mkdir bbr
cd bbr
wget https://raw.githubusercontent.com/jervisfm/rebbr/master/module/Makefile
wget https://raw.githubusercontent.com/jervisfm/rebbr/master/module/tcp_bbr.c

sudo apt-get -y install software-properties-common
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get -y install g++-4.9

sed -i 's/^/CC=gcc-4.9\n/' Makefile
make
sudo modprobe ./tcp_bbr.ko
sudo sysctl -w net.ipv4.tcp_congestion_control=bbr
