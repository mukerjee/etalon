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
 * Copyright 2005 by Myricom, Inc.  All rights reserved.                 *
 *************************************************************************/

#ifndef _mal_autodetect_h_
#define _mal_autodetect_h_

#ifdef	__cplusplus
extern "C"
{
#if 0
}				/* indent hack */
#endif
#endif

/************
 * Determine CPU based on what compiler is being used.
 ************/

#if defined MAL_CPU_alpha
#  define MAL_CPU_DEFINED 1
#elif defined MAL_CPU_lanai
#  define MAL_CPU_DEFINED 1
#elif defined MAL_CPU_mips
#  define MAL_CPU_DEFINED 1
#elif defined MAL_CPU_powerpc
#  define MAL_CPU_DEFINED 1
#elif defined MAL_CPU_powerpc64
#  define MAL_CPU_DEFINED 1
#elif defined MAL_CPU_sparc
#  define MAL_CPU_DEFINED 1
#elif defined MAL_CPU_sparc64
#  define MAL_CPU_DEFINED 1
#elif defined MAL_CPU_x86
#  define MAL_CPU_DEFINED 1
#elif defined MAL_CPU_x86_64
#  define MAL_CPU_DEFINED 1
#elif defined MAL_CPU_hppa
#  define MAL_CPU_DEFINED 1
#elif defined MAL_CPU_ia64
#  define MAL_CPU_DEFINED 1
#else
#  define MAL_CPU_DEFINED 0
#endif

#if !MAL_CPU_DEFINED
#  if defined _MSC_VER		/* Microsoft compiler */
#    if defined _M_IX86
#      define MAL_CPU_x86 1
#    elif defined _M_IA64
#      define MAL_CPU_ia64 1
#    elif defined _M_AMD64
#      define MAL_CPU_x86_64 1
#    elif defined _M_ALPHA
#      define MAL_CPU_alpha 1
#    else
#      error Could not determine CPU type. You need to modify mal_autodetect.h.
#    endif
#  elif defined __APPLE_CC__	/* Apple OSX compiler defines __GNUC__ */
#    if defined __ppc__ 	/* but doesn't support #cpu syntax     */
#      define MAL_CPU_powerpc 1
#    elif defined __ppc64__
#      define MAL_CPU_powerpc64 1
#    elif defined __i386__
#      define MAL_CPU_x86 1
#    elif defined __x86_64__
#      define MAL_CPU_x86_64 1
#    else
#      error Could not determine CPU type. You need to modify mal_autodetect.h.
#    endif
#  elif defined mips
#    define MAL_CPU_mips 1
#  elif defined(__GNUC__) || defined __linux__ || defined __PGI
#    if defined __alpha || defined __alpha__
#      define MAL_CPU_alpha 1
#    elif defined lanai || defined lanai3 || defined lanai7
#      define MAL_CPU_lanai 1
#    elif defined(powerpc64) || defined __powerpc64__
#      define MAL_CPU_powerpc64 1
#    elif defined(__ppc__) || defined __powerpc__ || defined powerpc
#      define MAL_CPU_powerpc 1
#    elif defined(_POWER) || defined _IBMR2
#      define MAL_CPU_powerpc 1
#    elif defined __ia64__
#      define MAL_CPU_ia64 1
#    elif defined sparc64 || defined __sparcv9
#      define MAL_CPU_sparc64 1
#    elif defined sparc || defined __sparc__
#      define MAL_CPU_sparc 1
#    elif defined __i386 || defined i386 || defined __i386__
#      define MAL_CPU_x86 1
#    elif defined __x86_64__
#      define MAL_CPU_x86_64 1
#    elif defined(CPU)   /* This is how vxWorks defines their CPUs */
#      if (CPU==PPC603)
#	 define MAL_CPU_powerpc 1
#      elif (CPU==PPC604)
#	 define MAL_CPU_powerpc 1
#      elif (CPU==PPC405)
#        define MAL_CPU_powerpc 1
#      else
#        error Could not determine CPU type. If this is VxWorks, you will need to modify mal_autodetect.h to add your cpu type.
#      endif
#    else
#      error Could not determine CPU type. You need to modify mal_autodetect.h.
#    endif
#  elif (defined (_POWER) && defined(_AIX))
#      define MAL_CPU_powerpc 1
#  elif (defined __powerpc__)
#      define MAL_CPU_powerpc 1
#  elif (defined (__DECC) || defined (__DECCXX)) && defined(__alpha)
#      define MAL_CPU_alpha 1
#  elif defined (__SUNPRO_C) || defined(__SUNPRO_CC)
#    if defined(sparc64) || defined(__sparcv9)
#      define MAL_CPU_sparc64 1
#    elif defined(sparc)
#      define MAL_CPU_sparc 1
#    elif defined i386
#      define MAL_CPU_x86 1
#    elif defined(__x86_64) || defined(__amd64)
#      define MAL_CPU_x86_64 1
#    endif
#  elif defined(__hppa) || defined(_PA_RISC1_1)
#      define MAL_CPU_hppa 1
#  else
#    error Could not determine CPU type. You need to modify mal_autodetect.h.
#  endif
#  undef MAL_CPU_DEFINED
#  define MAL_CPU_DEFINED 1
#endif

/** Define all undefined MAL_CPU switches to 0 to prevent problems
   with "gcc -Wundef" */

#ifndef MAL_CPU_alpha
#define MAL_CPU_alpha 0
#endif
#ifndef MAL_CPU_ia64
#define MAL_CPU_ia64 0
#endif
#ifndef MAL_CPU_hppa
#define MAL_CPU_hppa 0
#endif
#ifndef MAL_CPU_lanai
#define MAL_CPU_lanai 0
#endif
#ifndef MAL_CPU_mips
#define MAL_CPU_mips 0
#endif
#ifndef MAL_CPU_powerpc
#define MAL_CPU_powerpc 0
#endif
#ifndef MAL_CPU_powerpc64
#define MAL_CPU_powerpc64 0
#endif
#ifndef MAL_CPU_sparc
#define MAL_CPU_sparc 0
#endif
#ifndef MAL_CPU_sparc64
#define MAL_CPU_sparc64 0
#endif
#ifndef MAL_CPU_x86
#define MAL_CPU_x86 0
#endif
#ifndef MAL_CPU_x86_64
#define MAL_CPU_x86_64 0
#endif

/****************************************************************
 * Endianess
 ****************************************************************/

/* Determine endianness */

#if MAL_CPU_alpha
#  define MAL_CPU_BIGENDIAN 0
#elif MAL_CPU_lanai
#  define MAL_CPU_BIGENDIAN 1
#elif MAL_CPU_mips && MIPSEL
#  define MAL_CPU_BIGENDIAN 0
#elif MAL_CPU_mips
#  define MAL_CPU_BIGENDIAN 1
#elif MAL_CPU_powerpc
#  define MAL_CPU_BIGENDIAN 1
#elif MAL_CPU_powerpc64
#  define MAL_CPU_BIGENDIAN 1
#elif MAL_CPU_sparc
#  define MAL_CPU_BIGENDIAN 1
#elif MAL_CPU_sparc64
#  define MAL_CPU_BIGENDIAN 1
#elif MAL_CPU_hppa
#  define MAL_CPU_BIGENDIAN 1
#elif MAL_CPU_x86
#  define MAL_CPU_BIGENDIAN 0
#elif MAL_CPU_x86_64
#  define MAL_CPU_BIGENDIAN 0
#elif MAL_CPU_ia64
#  define MAL_CPU_BIGENDIAN 0
#else
#  error Could not determine endianness. You need to modify mal_autodetect.h.
#endif


/****************************************************************
 * OS
 ****************************************************************/

#ifndef MAL_OS_UDRV
#define MAL_OS_UDRV 0
#endif

#if MAL_CPU_lanai || MAL_OS_UDRV
/* nothing */
#elif defined __linux__ || defined __gnu_linux__
#define MAL_OS_LINUX 1
#elif defined _WIN32
#define MAL_OS_WINDOWS 1
#elif defined __FreeBSD__
#define MAL_OS_FREEBSD 1
#elif defined __APPLE__
#define MAL_OS_MACOSX 1
#elif defined sun || defined __sun__
#define MAL_OS_SOLARIS 1
#elif defined AIX || defined _AIX
#define MAL_OS_AIX 1
#else
#error cannot autodetect your Operating system
#endif

#ifndef MAL_OS_LINUX 
#define MAL_OS_LINUX 0
#endif
#ifndef MAL_OS_WINDOWS 
#define MAL_OS_WINDOWS 0
#endif
#ifndef MAL_OS_FREEBSD
#define MAL_OS_FREEBSD 0
#endif
#ifndef MAL_OS_MACOSX 
#define MAL_OS_MACOSX 0
#endif
#ifndef MAL_OS_SOLARIS
#define MAL_OS_SOLARIS 0
#endif
#ifndef MAL_OS_AIX
#define MAL_OS_AIX 0
#endif

/**********************************************************************/
/* Control import/export of symbols and calling convention.           */
/**********************************************************************/
#if !MAL_OS_WINDOWS
#  define MAL_FUNC(type) type
#  define MAL_VAR(type) type
#else
#  ifdef MAL_BUILDING_LIB
#    ifdef __cplusplus
#      define MAL_FUNC(type) extern "C" __declspec(dllexport) type __cdecl
#      define MAL_VAR(type) extern "C" __declspec(dllexport) type
#    else
#      define MAL_FUNC(type) __declspec(dllexport) type __cdecl
#      define MAL_VAR(type) __declspec(dllexport) type
#    endif
#  else
#    ifdef __cplusplus
#      define MAL_FUNC(type) extern "C" __declspec(dllimport) type __cdecl
#      define MAL_VAR(type) extern "C" __declspec(dllimport) type
#    else
#      define MAL_FUNC(type) __declspec(dllimport) type __cdecl
#      define MAL_VAR(type) __declspec(dllimport) type
#    endif
#  endif
#endif

#ifdef __cplusplus
#if 0
{				/* indent hack */
#endif
}
#endif

#endif /* ifndef _mal_autodetect_h_ */
