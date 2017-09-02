#!/bin/bash

if [ "z$1" = "z" ]; then
    echo "usage: $0 path/to/kernel/src"
    exit 1
fi

DST=$1

if [ ! -e $DST ]; then
    echo "error: $DST not found"
    exit 1
fi

echo "Step 1. transfer modified kernel source files"
for f in $(find ./kernel_patches/* -name '*.patch'); do
    patch -p1 < $f
done
