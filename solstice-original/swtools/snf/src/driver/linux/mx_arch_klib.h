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

/* This MX kernel lib code was originally contributed by
 * Brice.Goglin@ens-lyon.org (LIP/INRIA/ENS-Lyon) */

#ifndef _mx_arch_klib_h_
#define _mx_arch_klib_h_

#include "mx_arch.h"

/* kernel library symbols */

#define mx_klib_symbol(s) EXPORT_SYMBOL(s)

/* kernel library wrappers */

#define mx_klib_sprintf(args...) sprintf(args)
#define mx_klib_printf(args...) printk(args)
#define mx_klib_snprintf(args...) snprintf(args)
#define mx_klib_strlen(s) strlen(s)
#define mx_klib_strtol simple_strtol
#define mx_klib_strtoll simple_strtoll

static inline int
mx_klib_gettimeofday(struct timeval *tv, struct timezone *tz) {
  if (tv != NULL) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    jiffies_to_timeval(jiffies, tv);
#else
    do_gettimeofday(tv);
#endif
  }
  return 0;
}

/* klib mutex */

typedef struct semaphore mx_klib_mutex_t;

#define mx_klib_mutex_init(mutexp) sema_init(mutexp,1)
#define mx_klib_mutex_destroy(mutexp) do { /* nothing */ } while (0)
#define mx_klib_mutex_lock(mutexp) down(mutexp)
#define mx_klib_mutex_unlock(mutexp) up(mutexp)

/* klib thread */

typedef struct task_struct *mx_klib_thread_t;

static inline
int
mx_klib_thread_create(mx_klib_thread_t *threadp,
		      int (*func)(void *), void *arg, const char *name, int cpu) 
{
  struct task_struct *task;
  task = kthread_create(func, arg, name);
  if (IS_ERR(task)) {
    MX_WARN(("can't start kthread\n"));
    return PTR_ERR(task);
  }
  else {
    if (cpu >= 0)
      kthread_bind(task, (unsigned int) cpu);
    wake_up_process(task);
    *threadp = task;
    return 0;
  }
}

#define mx_klib_self current
#define mx_klib_thread_return_t int
#define mx_klib_thread_join(threadp) kthread_stop(*threadp)

static inline
void
mx_klib_thread_exit(mx_klib_thread_t threadp)
{
  while (!kthread_should_stop()) {
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();
  }
}
#define mx_klib_thread_init(threadp, name) do { } while (0)

/* copy routines */

int mx_copy_from_user_mm(char *kdst, uaddr_t usrc, struct mm_struct *src_mm, uint32_t len);
int mx_copy_to_user_mm(uaddr_t udst, const void *ksrc, struct mm_struct *dst_mm,  uint32_t len);


#endif /* _mx_arch_klib_h_ */
