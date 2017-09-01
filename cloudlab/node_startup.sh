#!/bin/bash

sudo apt-get update
sudo apt-get -y install iperf3 git zsh curl vim tmux python-pip xorg-dev libx11-dev htop git make g++ gcc emacs libcap-dev libidna11-dev nettle-dev

sudo ifconfig enp8s0 192.168.0.`hostname | cut -d'.' -f1 | cut -d'-' -f2 | awk '{print $1+1}'` netmask 255.255.255.0

ping host0 -c1
ping host1 -c1
ping host2 -c1
ping host3 -c1
ping router -c1

sudo arp -s host0 `arp | grep router | tr -s ' ' | cut -d' ' -f3`
sudo arp -s host1 `arp | grep router | tr -s ' ' | cut -d' ' -f3`
sudo arp -s host2 `arp | grep router | tr -s ' ' | cut -d' ' -f3`
sudo arp -s host3 `arp | grep router | tr -s ' ' | cut -d' ' -f3`

#sudo tcpdump -nnXXSs 0 -i enp8s0d1
