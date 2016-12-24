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

/* modifications for MX kernel lib made by
 * Brice.Goglin@ens-lyon.org (LIP/INRIA/ENS-Lyon) */

#ifndef _mx_pin_h_
#define _mx_pin_h_

#include "mal_int.h"

typedef uintptr_t mx_pin_type_t;

/* define memory types that are passed to communication routines */

#ifdef MAL_KERNEL
/*
 * kernel-level application may choose the memory context.
 */

/* the application must choose between one of these context */
#define MX_PIN_KERNEL		(1UL << 1)
#define MX_PIN_USER		mx_klib_memory_context()
#define MX_PIN_PHYSICAL		(1UL << 2)

/* OR this flag to if segments are full-pages (except the first and last ones) */
/* (keep it small to avoid collision with badly aligned mm_struct) */
#define MX_PIN_FULLPAGES	(1UL << 0)

/* internal flags */
#define MX_PIN_STREAMING	(1UL << 3)
#define MX_PIN_CONSISTENT	(1UL << 4)
#define MX_PIN_LIBMX_CTX	MX_PIN_KERNEL

/* flags that are relevant to low-level routines (pinning or copying) */
#define MX_PIN_CTX_TO_LOWLEVEL(x)	((x) & ~MX_PIN_FULLPAGES)

/* extract the pin type (not a memory context) */
#define MX_PIN_CTX_IS_USER_CTX(x)	(x >= (1UL << 5))
#define MX_PIN_CTX_TO_TYPE(x)	(MX_PIN_CTX_IS_USER_CTX(x) ? 0 : (x))

uintptr_t mx_klib_memory_context(void);

#else
/*
 * user-level only uses user-level context, the application has nothing to do.
 */

/* internal flags */
#define MX_PIN_LIBMX_CTX	MX_PIN_UNDEFINED

#endif

/* internal flags */
/* this value should be reachable with other flags or memory context
 * neither be 32/64 arch dependant. */
#define MX_PIN_UNDEFINED	(0xffffffff)

#endif /* _mx_pin_h_ */
