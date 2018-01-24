#!/bin/bash

rm /root/fpt_results.txt &> /dev/null
touch /root/fpt_results.txt
dir=hostname | cut -d'.' -f1
dd if=/dev/zero of=/root/temp_data.dat bs=128M count=1
/usr/local/hadoop/bin/hadoop fs -mkdir /test-$dir
sleep $((60 - $(date +%s) % 60))
for i in {1..1}
do
    { time /usr/local/hadoop/bin/hadoop fs -put -f /root/temp_data.dat /test-$dir/$i.dat ; } >> /root/fpt_results.txt 2>&1
done
