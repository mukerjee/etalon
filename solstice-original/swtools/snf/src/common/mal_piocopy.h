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
 * Copyright 2009 by Myricom, Inc.  All rights reserved.                 *
 *************************************************************************/

#ifndef __MAL_PIOCOPY__
#define __MAL_PIOCOPY__

#include "mal_cpu.h"

#define MAL_REQUEST_SIZE  64

/*************************
 * Generic copy routines *
 *************************/
static inline void 
mal_piocopy64_inline(void *to, uint64_t *from64, unsigned size, int fence)
{
  register volatile uint64_t *to64;
  uint64_t tmp;
  int i;

  mal_assert((size > 0)
	    && !(size & 7) && !((uintptr_t)to & 7) 
	    && !((uintptr_t)from64 & 7));

  to64 = (volatile uint64_t *) to;
  for (i = size / 8 - 1; i; i--)
    {
      *to64++ = *from64++;
    }
  tmp = *from64;
  if (fence)
    MAL_STBAR();
  *to64 = tmp;
  MAL_STBAR();
}

static inline void 
mal_piocopy32_inline(void *to, uint32_t *from32, unsigned size)
{
  register volatile uint32_t *to32;
  uint32_t tmp;
  int i;

  mal_assert((size > 0)
	    && !(size & 3) && !((uintptr_t)to & 3) 
	    && !((uintptr_t)from32 & 3));

  to32 = (volatile uint32_t *) to;
  for (i = size / 4 - 1; i; i--)
    {
      *to32++ = *from32++;
    }
  tmp = *from32;
  MAL_STBAR();
  *to32 = tmp;
  MAL_STBAR();
}

/******************************
 * Various ureq copy routines *
 ******************************/
#if MAL_PIOCOPY_WITH_SSE2
static inline void
mal_piocopy_ureq_with_sse2(void *to, uint64_t *from, int fence)
{
  if (fence) {
    __asm__ __volatile__(
		       "movdqa    (%1),%%xmm0\n\t"
		       "movdqa  16(%1),%%xmm1\n\t"
		       "movdqa  32(%1),%%xmm2\n\t"
		       "movdqa  48(%1),%%xmm3\n\t"
		       "movdqa  %%xmm0,   (%0)\n\t"
		       "movdqa  %%xmm1, 16(%0)\n\t"
		       "movdqa  %%xmm2, 32(%0)\n\t"
		       "movq  %%xmm3,  48(%0)\n\t"
		       "sfence\n\t"
		       "psrldq $8, %%xmm3 \n\t"
		       "movq  %%xmm3,  56(%0)\n\t"
		       "sfence\n\t"
		       ::"a"(to), "d"(from) : "memory" MAL_ASM_XMM_DEP);
  } else {
    __asm__ __volatile__(
		       "movdqa    (%1),%%xmm0\n\t"
		       "movdqa  16(%1),%%xmm1\n\t"
		       "movdqa  32(%1),%%xmm2\n\t"
		       "movdqa  48(%1),%%xmm3\n\t"
		       "sfence\n\t"
		       "movdqa  %%xmm0,   (%0)\n\t"
		       "movdqa  %%xmm1, 16(%0)\n\t"
       		       "movdqa  %%xmm2, 32(%0)\n\t"
       		       "movdqa  %%xmm3, 48(%0)\n\t"
       		       "sfence\n\t"
		       ::"a"(to), "d"(from) : "memory" MAL_ASM_XMM_DEP);
  }
}
#endif /* MAL_PIOCOPY_WITH_SSE2 */

#if MAL_PIOCOPY_WITH_ALTIVEC
static inline void
mal_piocopy_ureq_with_altivec(void *to, uint64_t *from)
{
  __asm__ __volatile__(
	"	sync\n"
	LVX	"0,  0, %4\n"
	LVX	"1,  0, %5\n"
	LVX	"2,  0, %6\n"
	LVX	"3,  0, %7\n"
	STVX	"0,  0, %0\n"
	STVX	"1,  0, %1\n"
	STVX	"2,  0, %2\n"
	STVX	"3,  0, %3\n"
	"	sync\n"
	:
	: "b"(to), 
	  "b"(((char*)to)+16), 
	  "b"(((char*)to)+32), 
	  "b"(((char*)to)+48), 
	  "b"(from), 
	  "b"(((char*)from)+16),
	  "b"(((char*)from)+32),
	  "b"(((char*)from)+48)
        );
}
#endif /* MAL_PIOCOPY_WITH_ALTIVEC */

#if MAL_PIOCOPY_WITH_PPC_ASM
static inline void
mal_piocopy_ureq_with_ppc_asm(void *to, uint64_t *from)
{
  __asm__ __volatile__(
	"	sync\n"
	LFD	"0,  0(%1)\n"
	LFD	"1,  8(%1)\n"
	LFD	"2, 16(%1)\n"
	LFD	"3, 24(%1)\n"
	LFD	"4, 32(%1)\n"
	LFD	"5, 40(%1)\n"
	LFD	"6, 48(%1)\n"
	LFD	"7, 56(%1)\n"
	STFD	"0,  0(%0)\n"
	STFD	"1,  8(%0)\n"
	STFD	"2, 16(%0)\n"
	STFD	"3, 24(%0)\n"
	STFD	"4, 32(%0)\n"
	STFD	"5, 40(%0)\n"
	STFD	"6, 48(%0)\n"
	STFD	"7, 56(%0)\n"
	"	sync\n"
	:
	: "b"(to), "b"(from)
        );
}
#endif /* MAL_PIOCOPY_WITH_PPC_ASM */

static inline void
mal_piocopy_ureq_without_fence(void *to, void *from)
{
#if MAL_PIOCOPY_WITH_SSE2
  mal_piocopy_ureq_with_sse2(to, from, 0);
#elif MAL_PIOCOPY_WITH_ALTIVEC
  mal_piocopy_ureq_with_altivec(to, from);
#elif MAL_PIOCOPY_WITH_PPC_ASM
  mal_piocopy_ureq_with_ppc_asm(to, from);
#else
  mal_piocopy64_inline(to, from, MAL_REQUEST_SIZE, 0);
#endif
}

static inline void
mal_piocopy_ureq(void *to, void *from)
{
#if MAL_PIOCOPY_WITH_SSE2
  mal_piocopy_ureq_with_sse2(to, from, 1);
#elif MAL_PIOCOPY_WITH_ALTIVEC
  mal_piocopy_ureq_with_altivec(to, from);
#elif MAL_PIOCOPY_WITH_PPC_ASM
  mal_piocopy_ureq_with_ppc_asm(to, from);
#else
  mal_piocopy64_inline(to, from, MAL_REQUEST_SIZE, 1);
#endif
}

static inline void
mal_piocopy_ureq_with_type_later(void *to, void *from)
{
#define TYPE32_POSITION_IN_UREQ MAL_REQUEST_SIZE/4-1
  uint32_t type32 = ((uint32_t*)from)[TYPE32_POSITION_IN_UREQ];
  ((uint32_t*)from)[TYPE32_POSITION_IN_UREQ] = 0;
  mal_piocopy_ureq_without_fence(to, from);
  ((uint32_t*)to)[TYPE32_POSITION_IN_UREQ] = type32;
  MAL_STBAR();
  /* restore the original type since the request might be reposted
   * later */
  ((uint32_t*)from)[TYPE32_POSITION_IN_UREQ] = type32;
}

static inline void
mal_piocopy_ureq_ze(void *to, void *from, mcp_ureq_type_t type)
{
  mal_assert(((uintptr_t) to & 0xf00) == 0);
  to = (char *)to + (type << 8);
  mal_piocopy_ureq_without_fence(to, from);
}

#endif /* __MAL_PIOCOPY__ */
