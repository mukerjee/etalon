#!/bin/bash

# make Click
# scp cloudlab_rsa mukerjee@aptxxx.apt.emulab.net:~/.ssh/id_rsa
# scp cloudlab_rsa.pub mukerjee@aptxxx.apt.emulab.net:~/.ssh/id_rsa.pub
git clone git@github.com:mukerjee/sdrt.git
cd sdrt/click
./configure --enable-user-multithread --disable-linuxmodule --enable-intel-cpu --enable-nanotimestamp --enable-dpdk
make

# run click
# n = 4? n = 3?
# http://dpdk-guide.gitlab.io/dpdk-guide/tuning/memory.html
# http://dpdk.org/doc/guides/nics/mlx4.html
sudo nice -n -20 ./userlevel/click --dpdk -c 0xFF -n 4 -- -j 8 ./conf/dpdk-hybrid-switch.click
