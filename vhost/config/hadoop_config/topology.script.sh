#!/bin/bash
# 10.1.3.16 --> /rack03
# 10.1.4.27 --> /rack04
echo $@ | xargs -n 1 | awk -F '.' '{printf "/rack%02d\n", $3}'
