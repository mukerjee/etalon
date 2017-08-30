#!/bin/bash

make accu_gtod
OUTPUT=accuracy.log
./accu_gtod -t 2 -d -e -p > accuracy.log
./accu_gtod -t 4 -d -e -p >> accuracy.log
./accu_gtod -t 8 -d -e -p >> accuracy.log
./accu_gtod -t 16 -d -e -p >> accuracy.log

cat accuracy.log
