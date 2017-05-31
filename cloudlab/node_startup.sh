#!/bin/bash

sudo apt-get update
sudo apt-get -y install iperf3 git zsh curl vim tmux python-pip xorg-dev libx11-dev htop git make g++ gcc emacs

ping host0 -c1
ping host1 -c1
ping router -c1

sudo arp -s host0 `arp | grep router | tr -s ' ' | cut -d' ' -f3`
sudo arp -s host1 `arp | grep router | tr -s ' ' | cut -d' ' -f3`
