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

#ifndef _mx_arch_io_h_
#define _mx_arch_io_h_

#if defined HAVE_SYS_ERRNO_H && !defined MAL_KERNEL
#include <sys/errno.h>
#endif

#ifdef MAL_KERNEL
/* These files may not be included in the kernel */
#undef HAVE_SYS_TYPES_H
#undef HAVE_NETINET_IN_H
#endif

#endif /* _mx_arch_io_h_ */

