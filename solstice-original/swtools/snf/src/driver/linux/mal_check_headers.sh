#!/bin/bash
# $1 is the kernel build dir
# $2 is the kernel source dir (might be the same)
# $3 is the generated check file

if [ $# -lt 3 ] ; then
	echo "$0 needs KSRC, KDIR and OUTPUT as arguments"
	exit -1
fi

KSRC=$1
KDIR=$2
OUTPUT=$3

# Find where the headers are (to avoid grepping at both places).
# Do not check for autoconf.h or version.h since these are in
# both the source and the build directory.
HEADERS=
if [ -f ${KSRC}/include/linux/kernel.h ] ; then
	HEADERS=$KSRC
else if [ -f ${KDIR}/include/linux/kernel.h ] ; then
	HEADERS=$KDIR
fi fi

# check that we found kernel headers
if [ -z ${HEADERS} ] ; then
	echo "Cannot find include/linux/kernel.h in ${KSRC} or ${KDIR}"
	exit -1
fi
echo "Using kernel headers in ${HEADERS}"

# generate the output file
rm -f ${OUTPUT}

# add the header
echo "#ifndef __MAL_CHECKS_H__" >> ${OUTPUT}
echo "#define __MAL_CHECKS_H__ 1" >> ${OUTPUT}
echo "" >> ${OUTPUT}

# what command line was used to generate with file
echo "/*" >> ${OUTPUT}
echo " * This file has been generated with mal_check_headers.sh on "`date` >> ${OUTPUT}
echo " * It has been called with:" >> ${OUTPUT}
echo " *   KSRC=${KSRC}" >> ${OUTPUT}
echo " *   KDIR=${KDIR}" >> ${OUTPUT}
echo " * It checked kernel headers in ${HEADERS}/include/" >> ${OUTPUT}
echo " */" >> ${OUTPUT}
echo "" >> ${OUTPUT}


# try to find __ioremap()  Somewhat tricky since they renamed the asm dir
# in the 2.6.24 timeframe
archname=`uname -m`
BITS=unknown
case ${archname} in
    i?86)
	ARCH=i386
	BITS=32
	;;
    x86_64)
	ARCH=x86_64
	BITS=64
	;;
    ia64)
	ARCH=ia64
	;;
    ppc64)
	ARCH=powerpc
	;;
    powerpc)
	ARCH=powerpc
	;;
    ppc)
	ARCH=powerpc
	;;
    *)
	ARCH=nopat
	;;
esac
if [ -d ${HEADERS}/arch/${ARCH}/include/asm ] ; then
    ASM=${HEADERS}/arch/${ARCH}/include/asm
    IOH=${HEADERS}/arch/${ARCH}/include/asm/io.h
elif [ -d ${HEADERS}/include/asm-${ARCH} ] ; then
    ASM=${HEADERS}/include/asm-${ARCH}
    IOH=${HEADERS}/include/asm-${ARCH}/io.h
elif [ -d ${HEADERS}/include/asm-x86 ] ; then
    ASM=${HEADERS}/include/asm-x86
    IOH=${HEADERS}/include/asm-x86/io_${BITS}.h
elif [ -d ${HEADERS}/include/asm-${archname} ] ; then
    ASM=${HEADERS}/include/asm-${archname}
    IOH=${HEADERS}/include/asm-${archname}/io.h
else
    IOH=/dev/null
fi




# test for linux/compile.h

if [ -f ${HEADERS}/include/linux/compile.h ] ; then
    echo "#define HAVE_LINUX_COMPILE_H 1"  >> ${OUTPUT} 
fi

if [ -f ${HEADERS}/include/asm/rmap.h ] ; then
    echo "#define HAVE_ASM_RMAP_H 1"  >> ${OUTPUT} 
fi

if [ -f ${ASM}/pgtable.h ] ; then
    grep pte_kunmap ${ASM}/pgtable.h > /dev/null && echo "#define HAVE_PTE_KUNMAP 1" >> ${OUTPUT} || true
    grep pte_offset_nested ${ASM}/pgtable.h > /dev/null && echo "#define HAVE_PTE_OFFSET_NESTED 1" >> ${OUTPUT} || true
    grep io_remap_pfn_range ${ASM}/pgtable.h > /dev/null && echo "#define HAVE_IO_REMAP_PFN_RANGE 1" >> ${OUTPUT} || true
    grep pte_offset_map_nested ${ASM}/pgtable.h > /dev/null && echo "#define HAVE_PTE_OFFSET_MAP_NESTED 1" >> ${OUTPUT} || true
fi

if [ -f ${ASM}/iommu.h ] ; then
    grep it_mapsize  ${ASM}/iommu.h  > /dev/null && echo "#define HAVE_IT_MAPSIZE 1"  >> ${OUTPUT} || true
fi

grep remap_pfn_range ${HEADERS}/include/linux/mm.h  > /dev/null \
    && echo "#define HAVE_REMAP_PFN_RANGE 1"  >> ${OUTPUT} || true
grep remap_page_range ${HEADERS}/include/linux/mm.h > /dev/null \
    && echo "#define HAVE_REMAP_PAGE_RANGE 1"  >> ${OUTPUT} || true
grep remap_page_range.*vma ${HEADERS}/include/linux/mm.h > /dev/null \
    && echo "#define HAVE_REMAP_PAGE_RANGE_5ARGS 1"  >> ${OUTPUT} || true
grep mmap_up_write ${HEADERS}/include/linux/mm.h > /dev/null \
    && echo "#define HAVE_MMAP_UP_WRITE 1"  >> ${OUTPUT} || true
grep do_munmap.*acct  ${HEADERS}/include/linux/mm.h > /dev/null \
    && echo "#define HAVE_DO_MUNMAP_4ARGS 1"  >> ${OUTPUT} || true
grep class_simple_device_add ${HEADERS}/include/linux/device.h > /dev/null\
    && echo "#define HAVE_CLASS_SIMPLE 1"  >> ${OUTPUT} || true
grep kthread_run ${HEADERS}/include/linux/kthread.h > /dev/null \
    && echo "#define HAVE_KTHREAD_RUN 1"  >> ${OUTPUT} || true

# skb_linearize had a gfp argument before 2.6.18
grep "skb_linearize *(.*, .* gfp)" ${HEADERS}/include/linux/skbuff.h > /dev/null \
  && echo "#define HAVE_SKB_LINEARIZE_2ARGS 1" >> ${OUTPUT} || true

grep "skb_transport_offset" ${HEADERS}/include/linux/skbuff.h > /dev/null \
  && echo "#define HAVE_SKB_TRANSPORT_OFFSET 1" >> ${OUTPUT} || true

grep "rcupdate" ${HEADERS}/include/linux/pid.h > /dev/null \
  && echo "#define HAVE_RCU_TASKLIST_LOCK 1" >> ${OUTPUT} || true

grep "current_euid" ${HEADERS}/include/linux/cred.h > /dev/null \
  && echo "#define HAVE_CURRENT_EUID 1" >> ${OUTPUT} || true

grep "netif_napi_add" ${HEADERS}/include/linux/netdevice.h > /dev/null \
  && echo "#define HAVE_NEW_NAPI 1" >> ${OUTPUT} || true

if egrep -q "atomic_add_negative" ${HEADERS}/include/linux/mm.h ; then : ; else
    echo "#define HAVE_OLD_PAGE_COUNT 1" >> ${OUTPUT} 
fi


# add the footer
echo "" >> ${OUTPUT}
echo "#endif /* __MAL_CHECKS_H__ */" >> ${OUTPUT}
