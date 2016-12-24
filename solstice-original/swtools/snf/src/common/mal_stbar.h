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

#ifndef _mal_stbar_h_
#define _mal_stbar_h_

/****************************************************************
 * Store synchronization barrier Macro
 ****************************************************************/

/****************
 * sparc
 ****************/

#if MAL_CPU_sparc || MAL_CPU_sparc64
/* asm_memory() is like asm(), but tells the compiler that the
   asm effects memory, if possible.  This macro is useful for hiding
   "#if" processing below. */
#ifdef __GNUC__
#define mx_asm_memory(x) asm (x : : : "memory")
#else
#define mx_asm_memory(x) asm (x)
#endif

/* Specify store barrier and read barrier asms for sparcv9 and sparcv8 */
#if defined __sparcv9
#define __MAL_STBAR() mx_asm_memory ("membar #MemIssue")
#define __MAL_READBAR() mx_asm_memory ("membar #LoadLoad | #LoadStore")
#define __MAL_WRITEBAR() mx_asm_memory ("membar #StoreLoad | #StoreStore")
#else
#define __MAL_STBAR() mx_asm_memory ("stbar")
#define __MAL_READBAR() /* Do nothing */
#define __MAL_WRITEBAR() mx_asm_memory ("stbar");
#endif

/* Use the barrier asms directly with the Gnu C compiler, but call a
   function instead with the Sun compiler for performance, since it
   disables optimization for any function containing an asm. */
#ifdef __GNUC__
#define MAL_STBAR( ) __MAL_STBAR();
#define MAL_READBAR() __MAL_READBAR()
#define MAL_WRITEBAR() __MAL_WRITEBAR()
#elif (defined (__SUNPRO_C) || defined (__SUNPRO_CC))
void mx__stbar(void);
void mx__readbar(void);
void mx__writebar(void);
#define MAL_STBAR( ) mx__stbar ()
#define MAL_READBAR() mx__readbar ()
#define MAL_WRITEBAR() mx__writebar ()
#endif

/****************
 * x86
 ****************/

#elif MAL_CPU_x86 || MAL_CPU_x86_64
#if defined _MSC_VER && defined _M_IX86
__inline void MAL_STBAR(void) { long l; __asm { xchg l, eax } }
#define MAL_READBAR() MAL_STBAR()
#define MAL_WRITEBAR() MAL_STBAR()
#define MAL_CPU_RELAX() YieldProcessor()
#elif defined _MSC_VER && defined _M_AMD64
#define MAL_STBAR _mm_mfence
#define MAL_READBAR() MAL_STBAR()
#define MAL_WRITEBAR() MAL_STBAR()
#define MAL_CPU_RELAX() YieldProcessor()
#elif defined __GNUC__ || defined __INTEL_COMPILER
#define MAL_STBAR() __asm__ __volatile__ ("sfence;": : :"memory")
#if MAL_CPU_x86_64 || MX_ENABLE_SSE2
#define MAL_READBAR() __asm__ __volatile__ ("lfence;": : :"memory")
#else
#define MAL_READBAR() __asm__ __volatile__ ("lock;addl $0,0(%%esp);": : : "memory");
#endif
#define MAL_WRITEBAR() __asm__ __volatile__ ("": : :"memory")
#define MAL_CPU_RELAX() __asm__ __volatile__ ("rep;nop": : :"memory")
#elif defined __SUNPRO_C || defined __SUNPRO_CC
#define __MAL_STBAR() asm("sfence")
#define __MAL_READBAR() asm("lfence")
#define __MAL_WRITEBAR() 
void mx__stbar(void);
void mx__readbar(void);
void mx_writebar(void);
#define MAL_STBAR( ) mx__stbar ()
#define MAL_READBAR() mx__readbar ()
#define MAL_WRITEBAR() mx__writebar ()
#elif defined __PGI
#error still need to implement sfence in for compiler.
#define MAL_STBAR()
#define MAL_READBAR() MAL_STBAR()
#define MAL_WRITEBAR() MAL_STBAR()
#error Do not know how to emit an sfence instruction with this compiler
#elif MAL_OS_SOLARIS
 
#define __MAL_STBAR() asm("sfence")
#define __MAL_READBAR() asm("lfence")

#endif

/****************
 * ia64
 ****************/

#elif MAL_CPU_ia64
#if defined _MSC_VER
#define MAL_STBAR( ) __mf ()
#define MAL_READBAR() MAL_STBAR()
#define MAL_WRITEBAR() MAL_STBAR()
#elif defined __INTEL_COMPILER
#include <ia64intrin.h>
#define MAL_STBAR( ) __mf ()
#define MAL_READBAR() MAL_STBAR()
#define MAL_WRITEBAR() MAL_STBAR()
#elif defined __GNUC__
#define MAL_STBAR() __asm__ volatile ("mf": : :"memory") /* ": : :" for C++ */
#define MAL_READBAR() MAL_STBAR()
#define MAL_WRITEBAR() MAL_STBAR()
#else
#error Do not know how to emit the "mf" instruction with this compiler.
#endif

/****************
 * alpha
 ****************/

#elif MAL_CPU_alpha
#ifdef __GNUC__
#define MAL_STBAR()  __asm__ volatile ("mb": : :"memory") /* ": : :" for C++ */
#define MAL_READBAR() __asm__ volatile ("mb": : :"memory")
#define MAL_WRITEBAR() __asm__ volatile ("wmb": : :"memory")
#elif defined __DECC || defined __DECCXX
#ifndef MAL_KERNEL
#include <c_asm.h>
#define MAL_STBAR() asm ("mb")
#define MAL_READBAR() asm ("mb")
#define MAL_WRITEBAR() asm ("wmb")
#else
#include <sys/types.h>
#define MAL_STBAR() mb()
#define MAL_READBAR() mb()
#define MAL_WRITEBAR() mb()
#endif
#else
#error Do not know how to emit the "mb" instruction with this compiler.
#endif

/****************
 * powerpc 
 * powerpc64
 ****************/

#elif MAL_CPU_powerpc || MAL_CPU_powerpc64
#ifdef __GNUC__
/* can't use -ansi for vxworks ccppc or this will fail with a syntax error */
#define MAL_STBAR()  __asm__ volatile ("sync": : :"memory") /* ": : :" for C++ */
#define MAL_READBAR() __asm__ volatile ("isync": : :"memory")
#define MAL_WRITEBAR() __asm__ volatile ("eieio": : :"memory")
#define MAL_BARRIER() __asm__ volatile ("": : :"memory")
#if MAL_CPU_powerpc
#define MAL_CPU_RELAX() MAL_BARRIER()
#else /* MAL_CPU_powerpc64 */
#define MAL_NOP() __asm__ volatile ("nop")
#define MAL_CPU_RELAX()		\
	do {			\
		MAL_NOP();	\
		MAL_NOP();	\
		MAL_BARRIER();	\
	} while(0)
#endif
#else
#if	MAL_OS_AIX
extern void __iospace_eieio(void); 
extern void __iospace_sync(void);  
#define MAL_STBAR()   __iospace_sync ()
#define MAL_READBAR() __iospace_sync ()
#define MAL_WRITEBAR() __iospace_eieio ()
#else	/* MAL_OS_AIX */
#error Do not know how to make a store barrier for this system
#endif	/* MAL_OS_AIX */
#endif

/****************
 * mips
 ****************/

#elif MAL_CPU_mips
#ifdef MAL_KERNEL
#define MAL_STBAR() __asm__ volatile ("sync": : :"memory");
#define MAL_READBAR() MAL_STBAR()
#define MAL_WRITEBAR() MAL_STBAR()
#else
#define MAL_STBAR() __asm__ volatile ("sync": : :"memory");
#define MAL_READBAR() MAL_STBAR()
#define MAL_WRITEBAR() MAL_STBAR()
#endif

/*****************
 * HP-PA RISC 
 *****************/
#elif MAL_CPU_hppa
#define MAL_STBAR()
#define MAL_READBAR() MAL_STBAR()
#define MAL_WRITEBAR() MAL_STBAR()

#endif /* various architectures */

#endif /* _mal_stbar_h_ */

