#!/bin/bash

if [ "$#" -ne 2 ] || ! [ -d "$1" ] || ! [ -d "$2" ]; then
    echo "usage: $0 path/to/clean-kernel/ path/to/dest-kernel/"
    exit 1
fi

CLEAN=${1%/}
DST=${2%/}

files=`find ./kernel_patches/* -name '*.patch'`

for f in $files; do
    f2=${f#./kernel_patches/}
    f2=${f2%.*}
    cp $CLEAN/$f2 $DST/$f2
    patch $DST/$f2 $f
done
