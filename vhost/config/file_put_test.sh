#!/bin/bash

dd if=/dev/zero of=/root/temp_data.dat bs=128M count=1
/usr/local/hadoop/bin/hadoop fs -mkdir /test
for i in {1..1}
do
    time /usr/local/hadoop/bin/hadoop fs -put -f /root/temp_data.dat /test/$i.dat
done
