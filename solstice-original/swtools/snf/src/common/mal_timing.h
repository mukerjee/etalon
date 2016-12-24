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

#ifndef _mal_timing_h_
#define _mal_timing_h_

#include "mal_auto_config.h"

#if !MAL_OS_WINDOWS && !defined(MAL_KERNEL)
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#define mal_gettimeofday(x,y) gettimeofday(x,y)
#endif

typedef uint64_t mal_cycles_t;

#if MAL_OS_WINDOWS
MAL_FUNC(int) mal_gettimeofday(struct timeval *tv, void *tz);
#endif

#if defined __GNUC__ && !(MAL_CPU_sparc || MAL_CPU_sparc64) && !defined(MAL_KERNEL)
mal_cycles_t mal_cycles_per_second(void);
double mal_seconds_per_cycle(void);

#if MAL_CPU_x86  || MAL_CPU_x86_64

static inline mal_cycles_t mal_get_cycles(void)
{
  unsigned l,h;
  __asm__ __volatile__("rdtsc": "=a" (l), "=d" (h));
  return l + (((mal_cycles_t)h) << 32);
}

#elif MAL_CPU_powerpc || MAL_CPU_powerpc64

static inline mal_cycles_t mal_get_cycles()
{ 
  mal_cycles_t t;
  unsigned tbu, tbl, tbu2;
  if (sizeof(void *) == 8) {
    asm volatile("mftb %0" : "=r" (t)); 
    return t;
  } else {
    do {
      asm volatile ("mftbu %0" : "=r" (tbu));
      asm volatile ("mftb %0" : "=r" (tbl));
      asm volatile ("mftbu %0" : "=r" (tbu2));
    } while (tbu != tbu2);
    return ((mal_cycles_t)tbu << 32) + tbl;
  }
}

#elif MAL_CPU_ia64

static inline mal_cycles_t mal_get_cycles()
{ 
  mal_cycles_t t;
  asm volatile ("mov %0=ar.itc" : "=r"(t));
  return t;
}

#elif MAL_CPU_alpha
static inline mal_cycles_t mal_get_cycles()
{ 
   mal_cycles_t t;
   asm volatile ("rpcc %0" : "=r" (t));
   /* according to the brown book, (I) 4-143, the lower
    * 32-bits are an unsigned, wrapping counter, but the
    * upper-32-bits are OS dependant.  So just use the
    * lower 32-bits */
   return t & 0xffffffffLL; 
}

#elif MAL_CPU_mips
static inline mal_cycles_t mal_get_cycles()
{
  unsigned int count;
  
  /* MIPS has 32-bit counter; what about wrap? */
  __asm__(".set 	push	\n"
	  ".set 	mips32r2\n"
	  "rdhwr $3, $30	\n"
	  "move  %0, $3   \n"
	  ".set pop" : "=r"(count) : : "$3");
  
  return (mal_cycles_t)count;
}

#else
#error mal_get_cycles not implemented
#endif

double mal_seconds_per_cycle(void);

mal_cycles_t mal_cycles_per_second(void);

#else /* !(GNU_C compiler & user-level & !sparc) */

#if MAL_OS_LINUX

#define mal_get_cycles() jiffies
#define mal_cycles_per_second() HZ

#elif MAL_OS_SOLARIS && !defined(MAL_KERNEL)

#define mal_get_cycles mal__get_cycles

mal_cycles_t mal__get_cycles(void);

double mal_seconds_per_cycle(void);

mal_cycles_t mal_cycles_per_second(void);

#elif !MAL_OS_WINDOWS

#include <sys/time.h>

static inline mal_cycles_t mal_get_cycles(void) 
{
  struct timeval t;
  mal_gettimeofday(&t,NULL);
  return (mal_cycles_t)t.tv_sec*1000000+t.tv_usec;
}

#ifndef MAL_KERNEL 
#define mal_seconds_per_cycle() 1e-6
#endif

#define mal_cycles_per_second() ((mal_cycles_t)1000000)

#else /* !MAL_OS_WINDOWS */
#include <windows.h>
static inline mal_cycles_t mal_get_cycles(void)
{
  LARGE_INTEGER li;

  QueryPerformanceCounter(&li);
  return li.QuadPart;
}

static inline double mal_seconds_per_cycle(void)
{
  LARGE_INTEGER li;

  QueryPerformanceFrequency(&li);
  return 1.0/(double)li.QuadPart;
}

static inline mal_cycles_t mal_cycles_per_second(void)
{
  LARGE_INTEGER li;

  QueryPerformanceFrequency(&li);
  return li.QuadPart;
}

#endif

#endif


void mal_cycles_counter_init(void);


#endif /* _mal_timing_h_ */
