#!/bin/bash

if [ "z$1" = "z" ]; then
    echo "usage: $0 path/to/kernel/src"
    echo "  e.g. $0 /home/yjq/ExtendKernel/linux-3.16.3/linux-3.16.3"
    exit 1
fi

DST=$1

if [ ! -e $DST ]; then
    echo "error: $DST not found"
    exit 1
fi

FILES="kernel/fork.c \
kernel/time.c \
kernel/time/timekeeping.c \
arch/x86/syscalls/syscall_64.tbl \
arch/x86/vdso/vclock_gettime.c \
include/uapi/asm-generic/unistd.h include/uapi/linux/sched.h \
include/linux/sched.h \
include/linux/init_task.h \
include/linux/syscalls.h \
include/net/sch_generic.h \
net/sched/sch_generic.c \
net/sched/sch_htb.c \
net/sched/act_police.c \
build_all.sh"

echo "Step 1. transfer modified kernel source files"
for f in $FILES; do
    cp -v $f $DST/$f
done
