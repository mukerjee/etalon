#!/bin/bash

if [ "$#" -ne 2 ] || ! [ -d "$1" ] || ! [ -d "$2" ]; then
    echo "usage: $0 path/to/clean-kernel/ path/to/dest-kernel/"
    exit 1
fi

CLEAN=${1%/}
DST=${2%/}

files=`find ./kernel_patches/* -name '*.patch'`

for f in $files; do
    f=${f#./kernel_patches/}
    f=${f%.*}
    b=`basename $f`
    cp $CLEAN/$f /tmp/$b
    patch /tmp/$b ./kernel_patches/$f.patch

    if cmp -s $DST/$f /tmp/$b
    then
        printf "\tskipping $DST/$f\n"
    else
	printf "\t"
        cp -v /tmp/$b $DST/$f
    fi
done
