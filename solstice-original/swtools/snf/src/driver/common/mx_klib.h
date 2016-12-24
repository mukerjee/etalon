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

/* This MX kernel lib code was originally contributed by
 * Brice.Goglin@ens-lyon.org (LIP/INRIA/ENS-Lyon) */

#ifndef _mx_klib_h_
#define _mx_klib_h_

#include "mx_arch.h"
#include "mx_malloc.h"
#include "mx_instance.h"
#include "mx_arch_klib.h"

/* forward declarations */
struct mx_endpt_state;

extern mx_klib_mutex_t mx_klib_lock;
#define MX_LOCK &mx_klib_lock

/* entry points */
int  myri_init_klib(void);
void myri_finalize_klib(void);

int myri_klib_open(mx_endpt_state_t **es);
int myri_klib_ioctl(mx_endpt_state_t *es, int cmd, void *p, size_t size);
int myri_klib_close(mx_endpt_state_t *es);
int myri_klib_set_endpoint(mx_endpt_state_t **es, uint32_t unit,
		           uint32_t endpoint, void *context, int cmd);
int myri_klib_map(mx_endpt_state_t *es, uintptr_t offset, void **ptr, mx_page_pin_t **pin);

/* std header replacement */
#define mx_malloc mal_malloc
#define mx_free mal_free
#define mx_calloc mal_calloc
#define mx_memcpy(x,y,z) bcopy(y,x,z)
#define mx_strlen(s) mx_klib_strlen(s)
#define mx_printf(args...) mx_klib_printf(args)
#define mx_snprintf(args...) mx_klib_snprintf(args)
#define mx_sprintf(args...) mx_klib_sprintf(args)
#define mx_strerrno "??"
#define mx_errno -1
#define mx_getpid() mx_klib_getpid()
#define mx_strtol mx_klib_strtol
#define mx_strtoll mx_klib_strtoll
#define mal_gettimeofday(x,y) mx_klib_gettimeofday(x,y)


#endif /* _mx_klib_h_ */
