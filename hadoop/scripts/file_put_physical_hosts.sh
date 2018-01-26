#!/bin/bash

rm $HOME/fpt_results.txt &> /dev/null
touch $HOME/fpt_results.txt
dir=`hostname | cut -d'.' -f1`
dd if=/dev/zero of=$HOME/temp_data.dat bs=128M count=1
/usr/local/hadoop/bin/hadoop fs -mkdir -p /test-$dir
sleep $((60 - $(date +%s) % 60))
for i in {1..1}
do
    { time /usr/local/hadoop/bin/hadoop fs -put -f $HOME/temp_data.dat /test-$dir/$i.dat ; } >> $HOME/fpt_results.txt 2>&1
done
