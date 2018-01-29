#!/bin/sh

sudo docker cp /users/mukerjee/HiBench h11:/root
sudo docker cp hibench.conf h11:/root/HiBench/conf
sudo docker cp hadoop.conf h11:/root/HiBench/conf
sudo docker cp slaves h11:/usr/local/hadoop/etc/hadoop
sudo docker cp masters h11:/usr/local/hadoop/etc/hadoop

