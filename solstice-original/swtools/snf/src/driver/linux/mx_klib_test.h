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

#ifndef _mx_klib_test_h_
#define _mx_klib_test_h_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/mman.h>
#include "mal_auto_config.h"
#include "myriexpress.h"


#ifdef HAVE_DO_MUNMAP_4ARGS
#define mx_do_munmap(mm, addr, len, acct) \
do_munmap(mm, addr, len, acct)
#else
#define mx_do_munmap(mm, addr, len, acct) \
do_munmap(mm, addr, len)
#endif

#define PRINT printk

static inline void *
alloc_buffer(size_t len, mx_pin_type_t pin_type)
{
  if  (pin_type == MX_PIN_PHYSICAL
       || pin_type == MX_PIN_KERNEL)
    return kmalloc(len, GFP_KERNEL);
  else if (pin_type == MX_PIN_USER)
    return (void *) do_mmap_pgoff(NULL, 0, len,
				  PROT_READ|PROT_WRITE,
				  MAP_PRIVATE|MAP_ANONYMOUS, 0);
  else
    return NULL;
}

static inline void
free_buffer(void *ptr, size_t len, mx_pin_type_t pin_type)
{
  if  (pin_type == MX_PIN_PHYSICAL
       || pin_type == MX_PIN_KERNEL)
    kfree(ptr);
  else if (pin_type == MX_PIN_USER)
    mx_do_munmap(current->mm, (unsigned long) ptr, len, 0);
}

static inline int
compare(void *ptr1, void *ptr2, size_t len, mx_pin_type_t pin_type)
{
  if  (pin_type == MX_PIN_PHYSICAL
       || pin_type == MX_PIN_KERNEL)
    return memcmp(ptr1, ptr2, pin_type);
  else if (pin_type == MX_PIN_USER) {
    void * kptr1, *kptr2;
    int ret = -1;
    kptr1 = kmalloc(len, GFP_KERNEL);
    if (!kptr1)
      goto out;
    kptr2 = kmalloc(len, GFP_KERNEL);
    if (!kptr2)
      goto out_with_kptr1;
    if (copy_from_user(kptr1, ptr1, len) || copy_from_user(kptr2, ptr2, len))
      goto out_with_kptr2;
    ret = memcmp(kptr1, kptr2, len);
   out_with_kptr2:
    kfree(kptr1);
   out_with_kptr1:
    kfree(kptr2);
   out:
    return ret;
  } else
    return -1;
}

static inline void
fill_pattern(char *dst, char *src, size_t len, uint32_t pin_type) {
  int i,j = 128;
  for(i=0; i<len; i+=j) {
    if (j > len-i)
      j = len-i;
  if  (pin_type == MX_PIN_PHYSICAL
       || pin_type == MX_PIN_KERNEL)
    memcpy(dst+i, src, j);
  }
}

#define DO_MODULE(init_func,cleanup_func)	\
module_init (init_func);			\
module_exit (cleanup_func);

#endif /* _mx_klib_test_h_ */
