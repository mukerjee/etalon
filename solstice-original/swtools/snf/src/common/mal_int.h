/*************************************************************************
 * The contents of this file are subject to the MYRICOM MYRINET          *
 * EXPRESS (MX) NETWORKING SOFTWARE AND DOCUMENTATION LICENSE (the       *
 * "License"); User may not use this file except in compliance with the  *
 * License.  The full text of the License can found in LICENSE.TXT       *
 *                                                                       *
 * Software distributed under the License is distributed on an "AS IS"   *
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See  *
 * the License for the specific language governing rights and            *
 * limitations under the License.                                        *
 *                                                                       *
 * Copyright 2003 - 2009 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#ifndef _mal_int_h_
#define _mal_int_h_

#include "mal_auto_config.h"

#if MAL_OS_LINUX
#ifdef MAL_KERNEL
#include <linux/types.h>
#include <linux/version.h>
typedef long intptr_t;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
typedef unsigned long uintptr_t;
#endif
#if BITS_PER_LONG == 32 || MAL_CPU_x86_64
#define MAL_PRI_64 "ll"
#define UINT64_C(value) value##ULL
#elif BITS_PER_LONG == 64
#define MAL_PRI_64 "l"
#define UINT64_C(value) value##UL
#else
#error 32 or 64?
#endif
#else
#include <inttypes.h>
#include <stdint.h>
#endif
#elif MAL_OS_MACOSX || MAL_OS_FREEBSD 
#ifdef MAL_KERNEL
#include <sys/types.h>
#if MAL_OS_MACOSX
#include <machine/limits.h>
#include <stdint.h>
#else
#include <sys/limits.h>
#endif
#if LONG_BIT == 32
#define MAL_PRI_64 "ll"
#elif LONG_BIT == 64
#define MAL_PRI_64 "l"
#endif
#else
/* MACOSX inttypes.h doesn't define SCNx64 unless __STDC_LIBRARY_SUPPORTED__ */
#if MAL_OS_MACOSX
#ifndef __STDC_LIBRARY_SUPPORTED__
#define __STDC_LIBRARY_SUPPORTED__
#endif
#endif
#include <inttypes.h>
#include <stdint.h>
#endif
#elif MAL_OS_SOLARIS
#include <inttypes.h>
#elif MAL_OS_AIX
#include <sys/types.h>
#elif MAL_OS_WINDOWS
typedef signed __int8      int8_t;
typedef signed __int16    int16_t;
typedef signed __int32    int32_t;
typedef signed __int64    int64_t;
typedef unsigned __int8   uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#ifdef _M_IX86
typedef unsigned __int32 uintptr_t;
#else
typedef unsigned __int64 uintptr_t;
#endif
#define MAL_PRI_64 "I64"
#define MAL_SCN_64 MAL_PRI_64
#define UINT64_C(value) value##ull
#elif MAL_OS_UDRV
#include <inttypes.h>
#include <stdint.h>
#include <unistd.h>
#else
#error Your platform is unsupported
#endif


#if defined MAL_PRI_64 && !defined PRIx64
/* we define MAL_PRI_64 no definition already present */
#define PRIx64 MAL_PRI_64 "x"
#define PRId64 MAL_PRI_64 "d"
#define PRIu64 MAL_PRI_64 "u"
#endif
#if defined MAL_SCN_64 && !defined SCNx64
/* we define MAL_PRI_64 no definition already present */
#define SCNx64 MAL_SCN_64 "x"
#define SCNd64 MAL_SCN_64 "d"
#define SCNu64 MAL_SCN_64 "u"
#endif

#ifdef MAL_KERNEL
#if MAL_OS_LINUX
typedef unsigned long uaddr_t;
#elif MAL_OS_MACOSX
#if MX_DARWIN_XX >= 8
typedef user_addr_t uaddr_t;
#else
typedef uintptr_t uaddr_t;
#endif
#elif MAL_OS_UDRV
typedef uint64_t uaddr_t;
#else
typedef uintptr_t uaddr_t;
#endif /* MAL_OS_ */
#endif /* MAL_KERNEL */

#endif /* _mal_int_h_ */
