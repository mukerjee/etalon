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
 * Copyright 2005 - 2009 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#if MAL_OS_WINDOWS
#include <windows.h>
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include "mal_timing.h"


static int
cmp_uint64(const void *ap,const void*bp)
{
  const mal_cycles_t *a = ap, *b = bp;
  return (*a == *b) ? 0 : (*a > *b) ? 1 : -1;
}

static mal_cycles_t cycles_per_second;
static double seconds_per_cycle;

#if !MAL_OS_WINDOWS
#undef mal_cycles_per_second
#undef mal_seconds_per_cycle
mal_cycles_t mal_cycles_per_second(void)
{
  if (!cycles_per_second) {
    mal_cycles_counter_init();
    if (!cycles_per_second) {
      /* TODO: FIXME after debug to mal */
      /* MX_WARN((stderr, "mal_cycles_per_second:gcc-compiled app with non-gcc lib")); */
    }
  }
  return cycles_per_second;
}

double mal_seconds_per_cycle(void)
{
  if (!cycles_per_second) {
    mal_cycles_counter_init();
    if (!cycles_per_second) {
      /* TODO: FIXME after debug to mal */
      /* MX_WARN(("mal_seconds_per_cycles:gcc-compiled app with non-gcc lib")); */
    }
  }
  return seconds_per_cycle;
}
#endif

#define NB_ITER 4
void mal_cycles_counter_init(void)
{
#if (defined __GNUC__ && !(MAL_CPU_sparc || MAL_CPU_sparc64)) || MAL_OS_SOLARIS
  mal_cycles_t cpc[NB_ITER];
  mal_cycles_t t1,t2;
  struct timeval tv1,tv2;
  int i;
  char *env;

  env = getenv("MAL_CYCLES_PER_SECOND");
  if (!env || !*env)
    env = getenv("MX_CYCLES_PER_SECOND");
  if (env) {
    sscanf(env,"%"PRId64,&cycles_per_second);
    if (cycles_per_second) {
      seconds_per_cycle = 1.0 / cycles_per_second;
      return;
    }
  }
  for (i=0;i<NB_ITER;i++) {
    t1 = mal_get_cycles();
    gettimeofday(&tv1,0);
    usleep(200000);
    t2 = mal_get_cycles();
    gettimeofday(&tv2,0);
    cpc[i] = ((t2-t1)*1000000ULL)/((tv2.tv_sec-tv1.tv_sec)*1000000ULL+tv2.tv_usec-tv1.tv_usec);
  }
  qsort(cpc,NB_ITER,sizeof(mal_cycles_t),cmp_uint64);
  cycles_per_second = cpc[2];
  seconds_per_cycle = 1.0 / cycles_per_second;
#endif
}

#if MAL_OS_SOLARIS && !defined(MAL_KERNEL)

#if MAL_CPU_x86_64
void mal__dummy_cycles(void)
{
  asm(".globl mal__get_cycles\n\t"
      "mal__get_cycles:\n\t"
      "rdtsc\n\t"
      "shlq    $32, %rdx\n\t"
      "orq     %rdx, %rax\n\t"
      "ret\n");
}
#elif MAL_CPU_x86
void mal__dummy_cycles(void)
{
  asm(".globl mal__get_cycles\n\t"
      "mal__get_cycles:\n\t"
      "rdtsc\n\t"
      "ret\n");
}
#else
mal_cycles_t mal__get_cycles(void)
{
  return gethrtime();
}
#endif

#endif /* MAL_OS_SOLARIS */


#if MAL_OS_WINDOWS

MAL_FUNC(int)
mal_gettimeofday(struct timeval* tv, void* tz)
{
#if 1
  static LARGE_INTEGER Frequency;
  LARGE_INTEGER CurrCounter;
  static int init = 0;
  FILETIME CurrentTime;
  static int UseHighResTimer = 1;
  LONGLONG tmp;
  tz = 0;

  if (!init) {
    if (!QueryPerformanceFrequency(&Frequency)) {
      UseHighResTimer = 0;
    }
    init = 1;
  }
  if (!tv)
    return -1;

  if (UseHighResTimer) {
    GetSystemTimeAsFileTime(&CurrentTime);
    /* don't include any function overhead */
    QueryPerformanceCounter(&CurrCounter);

    CurrCounter.QuadPart *= 10000000;
    CurrCounter.QuadPart /= Frequency.QuadPart;

    CurrentTime.dwHighDateTime = CurrCounter.HighPart;
    CurrentTime.dwLowDateTime = CurrCounter.LowPart;
  } else {
    GetSystemTimeAsFileTime(&CurrentTime);
  }

  tmp = ((LONGLONG) CurrentTime.dwHighDateTime << 32);
  tmp += (LONGLONG) CurrentTime.dwLowDateTime;

  tv->tv_sec = (long) (tmp / 10000000);
  tv->tv_usec = (long) (tmp - ((LONGLONG)(tv->tv_sec) * 10000000)) / 10;

  return 0;

#else

  FILETIME ft;
  ULARGE_INTEGER base1601;
  ULARGE_INTEGER base1970;

  if (tv != NULL) {
    GetSystemTimeAsFileTime (&ft);
    base1601.LowPart = ft.dwLowDateTime;
    base1601.HighPart = ft.dwHighDateTime;
    base1970.QuadPart = base1601.QuadPart - 116444736000000000;
    tv->tv_sec = (long)(base1970.QuadPart / 10000000);
    tv->tv_usec = (long)((base1970.QuadPart % 10000000) / 10);
  }
  return 0;
#endif
}

#endif /* MAL_OS_WINDOWS */

