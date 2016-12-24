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

#include "mal_thread.h"

#if MX_THREAD_SAFE

#if (MAL_OS_LINUX || MAL_OS_FREEBSD || MAL_OS_MACOSX || MAL_OS_SOLARIS || MAL_OS_UDRV)

#elif MAL_OS_WINDOWS

typedef struct thread_t
{
  void *(*start_routine)(void*);
  void *arg;
} thread_t;

unsigned __stdcall
mal_thread_create_helper(void* p)
{
  thread_t* t;
  
  t = (thread_t*) p;
  t->start_routine(t->arg);
  return 0;
}

MAL_FUNC(int)
mal_thread_create(mal_thread_t *thread, void *(*start_routine)(void *),
		  void *arg)
{
  unsigned thrdaddr;
  DWORD dwAffinityMask;

  
  thread->ti = (struct mal_threadinfo_t*)mal_malloc(sizeof (*thread->ti));
  thread->ti->start_routine = start_routine;
  thread->ti->arg = arg;
  thread->thread = (HANDLE)_beginthreadex(NULL, 0, 
					  mal_thread_create_helper,
					  thread->ti, 0, &thrdaddr);

  if (getenv ("MX_ENABLE_AFFINITY")) {
    int num_cpu;
    char buff[80];
    num_cpu = atoi (getenv ("MX_ENABLE_AFFINITY"));
    dwAffinityMask = (DWORD) num_cpu;
    if (!SetThreadAffinityMask (thread->thread,
                                (DWORD_PTR)dwAffinityMask)) {
      sprintf(buff, "SetProcessAffinityMask: %d\n", GetLastError());
      OutputDebugString (buff);
    }
    // Give the CPU the time to reschedule
    Sleep (0);
  }

  return 0;
}

int
mal_thread_join(mal_thread_t *t, void **thread_return)
{
  WaitForSingleObject(t->thread, INFINITE);
  mal_free(t->ti);
  return 0;
}

void
mal_thread_exit(void *retval)
{
  _endthreadex(0);
}

unsigned long
mal_thread_self(void)
{
  return (unsigned long)GetCurrentThreadId();
}

#else
#error no thread support for this OS
#endif

#else

typedef int foo;

#endif
