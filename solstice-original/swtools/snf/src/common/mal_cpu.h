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

#ifndef _mal_cpu_h_
#define _mal_cpu_h_

/***************************
 * x86-specific ASM checks *
 ***************************/
#if defined __GNUC__ && (MAL_CPU_x86 || MAL_CPU_x86_64)

#if (!defined MAL_KERNEL) && (MAL_ENABLE_SSE2 || MAL_CPU_x86_64)
#define MAL_PIOCOPY_WITH_SSE2 1
#if __GNUC__ >= 3 && defined __SSE2__
#define MAL_ASM_XMM_DEP ,"xmm0", "xmm1", "xmm2", "xmm3"
#else
#define MAL_ASM_XMM_DEP
#endif
#endif

#if MAL_CPU_x86_64
#define RBX "rbx"
#else
#define RBX "ebx"
#endif

static inline int mal__cpu_has_sse2(void)
{
  uint32_t eax,ecx,edx;
  __asm__("push %%"RBX";cpuid;pop %%"RBX : "=a" (eax), "=c" (ecx), "=d" (edx) : "0" (1));
  return (edx & (1 << 26)) != 0;
}

static inline int mal__cpu_is_intel(void)
{
  uint32_t eax, ebx, ecx, edx;
  uint64_t rbx_save;
  __asm__("mov %%"RBX", %4; cpuid; mov %%ebx,%3; mov %4,%%"RBX :
	  "=a" (eax), "=c" (ecx), "=d" (edx), "=m" (ebx), "=m" (rbx_save) : "0" (0));
  /* ebx edx ecx means "GenuineIntel" */
  return ebx == 0x756e6547 && edx == 0x49656e69 && ecx == 0x6c65746e;
}

#endif

/***************************
 * ppc-specific ASM checks *
 ***************************/
#if defined __GNUC__ && (MAL_CPU_powerpc || MAL_CPU_powerpc64) && !defined MAL_KERNEL

#if MAL_ENABLE_ALTIVEC
#define MAL_PIOCOPY_WITH_ALTIVEC 1
#else
#define MAL_PIOCOPY_WITH_PPC_ASM 1
#endif

/* Workaround MacOS non-GNU assembly */
#if MAL_OS_MACOSX
#define LFD "lfd        f"
#define STFD "stfd      f"
#define LVX "lvx        v"
#define STVX "stvx      v"
#else
#define LFD "lfd        "
#define STFD "stfd      "
#define LVX "lvx        "
#define STVX "stvx      "
#endif

#endif

/************
 * defaults *
 ************/
#ifndef MAL_PIOCOPY_WITH_SSE2
#define MAL_PIOCOPY_WITH_SSE2 0
#endif
#ifndef MAL_PIOCOPY_WITH_ALTIVEC
#define MAL_PIOCOPY_WITH_ALTIVEC 0
#endif
#ifndef MAL_PIOCOPY_WITH_PPC_ASM
#define MAL_PIOCOPY_WITH_PPC_ASM 0
#endif

#endif /* _mal_cpu_h_ */
