#!/bin/bash

NUM_RACKS=8

for i in `seq 1 $NUM_RACKS`
do
    echo $i
    scp -o StrictHostKeyChecking=no ~/hadoop-MATT-2.7.5.tar.gz host$i:/sdrt/vhost/hadoop-2.7.5.tar.gz
    # scp -o StrictHostKeyChecking=no ~/hadoop-2.7.5.tar.gz host$i:/sdrt/vhost/hadoop-2.7.5.tar.gz
    scp -o StrictHostKeyChecking=no ~/hadoop-MATT-SDRT-2.7.5.tar.gz host$i:/sdrt/vhost/hadoop-SDRT-2.7.5.tar.gz
    # scp -o StrictHostKeyChecking=no ~/hadoop-SDRT-2.7.5.tar.gz host$i:/sdrt/vhost/
    scp -o StrictHostKeyChecking=no ~/HiBench.tar.gz host$i:/sdrt/vhost/
done
