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

#ifndef _mx_misc_h_
#define _mx_misc_h_

#define MYRI_STR(x) _MYRI_STR(x)
#define _MYRI_STR(x) #x

#define MYRI_DRIVER_STR MYRI_STR(MYRI_DRIVER)


#define MX_PRINT_ONCE(s)			       \
  do {						       \
    static int _deja_vu;			       \
    if (!_deja_vu++)				       \
      MX_PRINT(s);				       \
  } while (0)


/* macros to stuff uint64_t's into dma descriptors */

#define MX_HIGHPART_TO_U32(X) \
(sizeof (X) == 8) ? ((uint32_t)((uint64_t)(X) >> 32)) : (0)

#define MX_LOWPART_TO_U32(X) ((uint32_t)(X))

#define MX_U32_TO_HIGHPART(X, Y) \
(sizeof (Y) == 8) ? (((uint64_t)X << 32)) : (0)


/* Virtual Pages (MCP always uses 4K pages) */
#define MX_VPAGE_MASK      (~(uaddr_t)(MX_VPAGE_SIZE - 1))
#define MX_ATOVP(x)        ((uaddr_t)(x) >> MX_VPAGE_SHIFT)
#define MX_VPTOA(x)        ((uaddr_t)(x) << MX_VPAGE_SHIFT)

/* Host Pages */
#ifdef PAGE_SIZE

#if PAGE_SIZE == 4096
#define MX_PAGE_SHIFT 12
#elif PAGE_SIZE == 8192
#define MX_PAGE_SHIFT 13
#elif PAGE_SIZE == 16384
#define MX_PAGE_SHIFT 14
#elif PAGE_SIZE == 65536
#define MX_PAGE_SHIFT 16
#else 
#error "unsupported page size"
#endif /* PAGE_SIZE == 4096 */

#define MX_PAGE_SIZE      (1UL << MX_PAGE_SHIFT)
#define MX_PAGE_OFFSET(x) ((x) & (uaddr_t)(MX_PAGE_SIZE - 1))
#define MX_PAGE_TRUNC(x)  ((x) & ~(uaddr_t)(MX_PAGE_SIZE - 1))
#define MX_PAGE_ALIGN(x)  MX_PAGE_TRUNC((x) + MX_PAGE_SIZE - 1)
#define MX_ATOP(x)        ((uaddr_t)(x) >> MX_PAGE_SHIFT)
#define MX_PTOA(x)        ((uaddr_t)(x) << MX_PAGE_SHIFT)


#define MX_IS_POWER_OF_TWO(x) 	(!((x) & ((x) - 1)))

/* note that MX_ROUND_ROUTE(8) == 16 on purpose */
#define MX_ROUND_ROUTE(x)	(((x) + 8) & ~7)
#endif /*PAGE_SIZE*/

#define mx_mem_check(a) do { if (!(a)) goto handle_enomem; } while (0)

#endif /* _mx_misc_h_ */
