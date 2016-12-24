#!/bin/bash

usage() {
    echo "`basename $0` [--kernel <kernel version>] [<snf base dir>]"
    exit $1
}

if test "$1" = "--help"; then
    usage 0
fi

if test "$1" = "--kernel"; then
    shift || usage 1
    kver=$1
    shift || usage 1
else 
    kver=`uname -r`
fi

if test ! -d "/lib/modules/$kver"; then
    echo "'/lib/modules/$kver/build' doesn't exits. Check your kernel version"
    echo "and/or that you have installed the correct packages to enable compiling"
    echo "of kernel modules."
    usage
fi

if test -n "$1"; then
    prefix=$1
elif test -n "$SNF_PREFIX"; then
    prefix=$SNF_PREFIX
else
    dirname=`dirname $0`
    prefix=`cd $dirname/..; pwd`
fi

if test ! -e $prefix/sbin/rebuild.sh; then
  echo "'$prefix' doesn't seem to be the snf base directory"
  echo "If you specified the prefix on the command line or through"
  echo "SNF_PREFIX, please check your path."
  usage 1
fi

if test -n "$MYRI_MCP" -a -f "$prefix/src/driver/common/$MYRI_MCP/mcp_array_10g_prebuilt.c"; then
  myri_mcp=$MYRI_MCP
  cp $prefix/src/driver/common/$MYRI_MCP/mcp_array_10g_prebuilt.c $prefix/src/driver/linux/kbuild/mcp_array_10g.c
else
  myri_mcp=default
fi
echo "Building myri_snf.ko for `uname -r` in $prefix/src with MYRI_MCP=$myri_mcp"
#regenerate mal checks
(
  set -x
  set -e
  $prefix/src/driver/linux/mal_check_headers.sh /lib/modules/`uname -r`/source \
  	/lib/modules/`uname -r`/build $prefix/src/driver/linux/mal_checks.h
  if test -d "$prefix/src/libsnf"; then
  	libsnf_srcs=`find $prefix/src/libsnf -maxdepth 1 -type f -name '*.c' | sed -e 's,.*/,,' -e 's/\.c$/.o/'`
  fi
  
  #replace Kbuild file
  cat >$prefix/src/driver/linux/kbuild/Kbuild <<EOF 
EXTRA_CFLAGS += -DMAL_KERNEL -DMX_THREAD_SAFE=1
EXTRA_CFLAGS += -DMYRI_DRIVER=myri_snf
EXTRA_CFLAGS += -I$prefix/src/driver/linux
EXTRA_CFLAGS += -I$prefix/src/driver/common
EXTRA_CFLAGS += -I$prefix/src/common
EXTRA_CFLAGS += -I$prefix/src/common
EXTRA_CFLAGS += -I$prefix/src/common/fapi
EXTRA_CFLAGS += -I$prefix/src/libsnf
EXTRA_CFLAGS += -I$prefix/include
EXTRA_CFLAGS += -g -Wall -Werror
obj-m = myri_snf.o
myri_snf-objs = mx.o mx_ether.o mx_common.o mx_instance.o mx_register.o mx_lanai_command.o mcp_wrapper_common.o mx_lx.o mx_lz.o mx_ether_common.o myri_snf_common.o mx_klib.o kraw.o myri_ptp_common.o $libsnf_srcs
ifndef CONFIG_ZLIB_INFLATE
EXTRA_CFLAGS += -I$prefix/src/driver/zlib
myri_snf-objs += adler32.o crc32.o infblock.o infcodes.o inffast.o inflate.o inftrees.o infutil.o trees.o uncompr.o zutil.o
endif
EOF
  make -C /lib/modules/`uname -r`/build M=$prefix/src/driver/linux/kbuild clean
  make -C /lib/modules/`uname -r`/build M=$prefix/src/driver/linux/kbuild
  cp $prefix/src/driver/linux/kbuild/myri_snf.ko $prefix/sbin/
  sed -e "s,MYRI_MODULE_DIR=/opt/snf,MYRI_MODULE_DIR=$prefix," < $prefix/sbin/myri_start_stop.orig > $prefix/sbin/myri_start_stop
  chmod +x $prefix/sbin/myri_start_stop
) > /tmp/myri_snf.log 2>&1
if test $? != 0; then
    echo "ERROR building myri_snf.ko, output available in /tmp/myri_snf.log"
else
    echo "SNF driver in $prefix/sbin"
fi

