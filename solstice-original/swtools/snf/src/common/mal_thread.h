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

#ifndef MAL_THREAD_H
#define MAL_THREAD_H

#ifndef MX_AUTO_CONFIG_H
#endif

#ifdef MAL_KERNEL

#include "mx_klib.h"

#define MAL_MUTEX_T mx_klib_mutex_t
#define MAL_MUTEX_INIT(m) mx_klib_mutex_init(m)
#define MAL_MUTEX_DESTROY(m) mx_klib_mutex_destroy(m)
#define MAL_MUTEX_LOCK(m) mx_klib_mutex_lock(m)
#define MAL_MUTEX_UNLOCK(m) mx_klib_mutex_unlock(m)

#define MAL_THREAD_T mx_klib_thread_t
#define MAL_THREAD_RETURN_T mx_klib_thread_return_t
#define MAL_THREAD_CREATE(threadp, fctn, arg, name) \
mx_klib_thread_create(threadp, fctn, arg, name, -1)
#define MAL_THREAD_CREATE_BIND(threadp, fctn, arg, name, bind) \
mx_klib_thread_create(threadp, fctn, arg, name, bind)
#define MAL_THREAD_JOIN(threadp) mx_klib_thread_join(threadp)
#define MAL_THREAD_EXIT_WITH_NOTHING(threadp) mx_klib_thread_exit(threadp)
#define MAL_THREAD_SELF() mx_klib_thread_self
#define MAL_THREAD_INIT(threadp, name) mx_klib_thread_init(threadp, name)

#define MAL_EVENT_T mx_sync_t
#define MAL_EVENT_INIT(eventp) mx_sync_init(eventp, NULL, -1, "mx_klib_event")
#define MAL_EVENT_DESTROY(eventp) mx_sync_destroy(eventp)
#define MAL_EVENT_SIGNAL(eventp) mx_wake(eventp)

#define MAL_EVENT_WAIT(eventp, lock) do {	\
  mx_klib_mutex_unlock(lock);			\
  mx_sleep(eventp, MX_MAX_WAIT, MX_SLEEP_INTR);	\
  mx_klib_mutex_lock(lock);			\
} while(0)                                                                      

#define MAL_EVENT_WAIT_TIMEOUT(eventp, lock, msecs)	\
do {							\
  mx_klib_mutex_unlock(lock);				\
  mx_sleep(eventp, msecs, MX_SLEEP_INTR);		\
  mx_klib_mutex_lock(lock);				\
} while(0)                                                                      

/* semaphore useless now */

#else /* MAL_KERNEL */

#if MX_THREAD_SAFE

#if (MAL_OS_LINUX || MAL_OS_FREEBSD || MAL_OS_MACOSX || MAL_OS_SOLARIS || MAL_OS_UDRV)

#include <pthread.h>
#include <sys/time.h>

#define MAL_MUTEX_T pthread_mutex_t
#define MAL_MUTEX_INIT(lock) pthread_mutex_init(lock,NULL)
#define MAL_MUTEX_DESTROY(lock) pthread_mutex_destroy(lock)
#define MAL_MUTEX_LOCK(lock) pthread_mutex_lock(lock)
#define MAL_MUTEX_UNLOCK(lock) pthread_mutex_unlock(lock)

#define MAL_THREAD_T pthread_t
#define MAL_THREAD_RETURN_T void*
#define MAL_THREAD_CREATE(threadp,fctn,arg,name) \
pthread_create(threadp, NULL, fctn, arg)
#define MAL_THREAD_CREATE_BIND(threadp,fctn,arg,name,cpu) \
pthread_create(threadp, NULL, fctn, arg)
#define MAL_THREAD_JOIN(threadp) pthread_join(*threadp, NULL)
#define MAL_THREAD_EXIT_WITH_NOTHING(threadp) pthread_exit(0); return 0
#define MAL_THREAD_SELF() pthread_self()
#define MAL_THREAD_EQUAL(t1, t2) pthread_equal(t1, t2)
#define MAL_THREAD_CANCEL(threadp) pthread_cancel(*threadp)
#define MAL_THREAD_INIT(threadp, name) do { /* nothing */ } while (0)

#if MAL_OS_SOLARIS
#include <semaphore.h>
#endif

#define MAL_SEMAPHORE_T sem_t
#define MAL_SEMAPHORE_WAIT(sema) sem_wait(sema)
#define MAL_SEMAPHORE_INIT(sema,value) sem_init(sema,0,value)
#define MAL_SEMAPHORE_DESTROY(sema) sem_destroy(sema)
#define MAL_SEMAPHORE_POST(sema) sem_post(sema)

#define MAL_EVENT_T pthread_cond_t
#define MAL_EVENT_INIT(eventp) pthread_cond_init (eventp, NULL)
#define MAL_EVENT_SIGNAL(eventp) pthread_cond_signal (eventp)
#define MAL_EVENT_WAIT(eventp, lock) pthread_cond_wait (eventp, lock)
#define MAL_EVENT_WAIT_TIMEOUT(eventp, lock, milli)		\
do {								\
  struct timeval tv;						\
  struct timespec abstime;					\
  gettimeofday(&tv, NULL);					\
  abstime.tv_sec = tv.tv_sec + (milli/1000);			\
  abstime.tv_nsec = tv.tv_usec * 1000 + (milli%1000) * 1000000;	\
  pthread_cond_timedwait(eventp, lock, &abstime);		\
} while (0)
#define MAL_EVENT_DESTROY(eventp) pthread_cond_destroy (eventp)

#elif MAL_OS_WINDOWS

#include <windows.h>
#include <process.h>

#define MX_USE_WIN32_MUTEX 0
#if MX_USE_WIN32_MUTEX
#define MAL_MUTEX_T HANDLE
#define MAL_MUTEX_INIT(lock) *lock=CreateMutex(NULL,FALSE,NULL)
#define MAL_MUTEX_DESTROY(lock) CloseHandle(*lock)
#define MAL_MUTEX_LOCK(lock) WaitForSingleObject(*lock, INFINITE)
#define MAL_MUTEX_UNLOCK(lock) ReleaseMutex(*lock)
#else
#define MAL_MUTEX_T CRITICAL_SECTION
#define MAL_MUTEX_INIT(lock) InitializeCriticalSection(lock)
#define MAL_MUTEX_DESTROY(lock) DeleteCriticalSection(lock)
#define MAL_MUTEX_LOCK(lock) EnterCriticalSection(lock)
#define MAL_MUTEX_UNLOCK(lock) LeaveCriticalSection(lock)
#endif

typedef struct mal_threadinfo_t
{
  void *(*start_routine)(void*);
  void *arg;
} mal_threadinfo_t;

typedef struct mal_thread_t
{
  HANDLE thread;
  mal_threadinfo_t *ti;
} mal_thread_t;

MAL_FUNC(int) mal_thread_create(mal_thread_t *thread,
		      void *(*start_routine)(void *), void *arg);
int mal_thread_join(mal_thread_t *t, void **thread_return);
void mal_thread_exit(void *retval);

#define MAL_THREAD_T mal_thread_t
#define MAL_THREAD_RETURN_T void*
#define MAL_THREAD_CREATE(thread,fctn,arg,name)                             \
mal_thread_create(thread, fctn, arg)
#define MAL_THREAD_JOIN(thread) mal_thread_join(thread, NULL)
#define MAL_THREAD_EXIT_WITH_NOTHING(retval) mal_thread_exit(retval); return 0
#define MAL_THREAD_SELF() mal_thread_self()
#define MAL_THREAD_EQUAL(t1, t2) (t1 == t2)
#define MAL_THREAD_INIT(threadp, name) do { /* nothing */ } while (0)

#define MAL_SEMAPHORE_T HANDLE
#define MAL_SEMAPHORE_WAIT(sema) WaitForSingleObject(*sema, INFINITE)
#define MAL_SEMAPHORE_INIT(sema,value)                                 \
*sema = CreateSemaphore(NULL, value, 0x7FFFFFFFL, NULL)
#define MAL_SEMAPHORE_DESTROY(sema) CloseHandle(*sema)
#define MAL_SEMAPHORE_POST(sema) ReleaseSemaphore(*sema, 1, NULL)

#define MAL_EVENT_T HANDLE
#define MAL_EVENT_INIT(event) *event=CreateEvent(NULL,FALSE,FALSE,NULL)
#define MAL_EVENT_SIGNAL(event) SetEvent(*event)
#if MX_USE_WIN32_MUTEX
#define MAL_EVENT_WAIT(event,lock) do {			\
   SignalObjectAndWait(*(lock),*(event), INFINITE,FALSE);	\
   WaitForSingleObject(*(lock), INFINITE);		\
  } while (0);
#define MAL_EVENT_WAIT_TIMEOUT(event,lock,milli) do {	\
   SignalObjectAndWait(*(lock), *(event), milli, FALSE);	\
   WaitForSingleObject(*(lock), INFINITE);		\
  } while (0);
#else
#define MAL_EVENT_WAIT(event,lock) do {           \
  MAL_MUTEX_UNLOCK(lock);                         \
  WaitForSingleObject(*(event), INFINITE); \
  MAL_MUTEX_LOCK(lock);                           \
  } while (0);
#define MAL_EVENT_WAIT_TIMEOUT(event,lock,milli) do {    \
    MAL_MUTEX_UNLOCK(lock);                              \
    WaitForSingleObject(*(event), milli);         \
    MAL_MUTEX_LOCK(lock);                                \
  } while (0);
#endif
#define MAL_EVENT_DESTROY(event) CloseHandle(*event)

#else
#error no thread support for this OS
#endif

#else

#define MAL_MUTEX_T int
#define MAL_MUTEX_INIT(x)
#define MAL_MUTEX_DESTROY(x)
#define MAL_MUTEX_LOCK(x)
#define MAL_MUTEX_UNLOCK(x)

#define MAL_THREAD_T int
#define MAL_THREAD_RETURN_T int
#define MAL_THREAD_CREATE(threadp,fctn,arg,name) 0
#define MAL_THREAD_JOIN(threadp)
#define MAL_THREAD_EXIT_WITH_NOTHING(threadp) return 0
#define MAL_THREAD_SELF()
#define MAL_THREAD_INIT(threadp, name)

#define MAL_SEMAPHORE_T int
#define MAL_SEMAPHORE_WAIT(sema)
#define MAL_SEMAPHORE_INIT(sema,value)
#define MAL_SEMAPHORE_DESTROY(sema)
#define MAL_SEMAPHORE_POST(sema)

#define MAL_EVENT_T int
#define MAL_EVENT_INIT(eventp)
#define MAL_EVENT_SIGNAL(eventp)
#define MAL_EVENT_WAIT(eventp,lock)
#define MAL_EVENT_WAIT_TIMEOUT(eventp,lock,milli)
#define MAL_EVENT_DESTROY(eventp)

#endif

#endif /* MAL_KERNEL */

#endif /* MX_THREAD_H */
