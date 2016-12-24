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
 * Copyright 2003 - 2004 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#ifndef MX_BYTESWAP_H
#define MX_BYTESWAP_H

#include "mal_int.h"

/* Get OS specific version of ntoh{s,l} */
#ifndef MAL_KERNEL
#if MAL_OS_MACOSX
#include <machine/endian.h>
#elif MAL_OS_LINUX || MAL_OS_FREEBSD || MAL_OS_UDRV
#include <netinet/in.h>
#elif MAL_OS_SOLARIS
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>
#elif MAL_OS_WINDOWS
#include <winsock2.h>
#else
#error
#endif
#endif

/* optimized version of swab */
#if defined MAL_KERNEL && MAL_OS_LINUX
#include <asm/byteorder.h>
#define mx_swab16 swab16
#define mx_swab32 swab32
#elif MAL_OS_LINUX
#include <byteswap.h>
#define mx_swab16 bswab_16
#define mx_swab32 bswab_32
#endif

#define mx_constant_swab32(x) \
(uint32_t)((((uint32_t)(x) >> 24) &   0xff) | \
           (((uint32_t)(x) >>  8) & 0xff00) | \
           (((uint32_t)(x) & 0xff00) <<  8) | \
           (((uint32_t)(x) &   0xff) << 24))

#define mx_constant_swab16(x) \
(uint16_t)((((uint16_t)(x) >> 8) & 0xff) | \
           (((uint16_t)(x) & 0xff) << 8))

#ifndef mx_swab32
#define mx_swab32 mx_constant_swab32
#define mx_swab16 mx_constant_swab16
#endif


#if MAL_CPU_BIGENDIAN
#define mx_constant_htonl(x) (x)
#define mx_constant_ntohl(x) (x)
#define mx_constant_htons(x) (x)
#define mx_constant_ntohs(x) (x)
#else
#define mx_constant_htonl(x) mx_constant_swab32(x)
#define mx_constant_ntohl(x) mx_constant_swab32(x)
#define mx_constant_htons(x) mx_constant_swab16(x)
#define mx_constant_ntohs(x) mx_constant_swab16(x)
#endif

#if MAL_CPU_BIGENDIAN
#define mx_htonll(x) (x)
#define mx_ntohll(x) (x)
#define mx_htole_u16 mx_swab16
#define mx_htole_u32 mx_swab32
#define mx_letoh_u16 mx_swab16
#define mx_letoh_u32 mx_swab32
#else
#define mx_htonll(x) \
((((x) >> 56) &       0xff) + \
 (((x) >> 40) &     0xff00) + \
 (((x) >> 24) &   0xff0000) + \
 (((x) >> 8)  & 0xff000000) + \
 (((x) & 0xff000000) <<  8) + \
 (((x) &   0xff0000) << 24) + \
 (((x) &     0xff00) << 40) + \
 (((x) &       0xff) << 56))
#define mx_ntohll(x) mx_htonll(x)
#define mx_htole_u16(x) (x)
#define mx_htole_u32(x) (x)
#define mx_letoh_u16(x) (x)
#define mx_letoh_u32(x) (x)
#endif

#endif
