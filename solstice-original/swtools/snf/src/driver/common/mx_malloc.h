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

#ifndef _mx_malloc_h_
#define _mx_malloc_h_

void *mx_kmalloc (size_t len, uint32_t flags);
void *mx_kmalloc_node (size_t len, uint32_t flags, int node_id);
void  mx_kfree (void *ptr);

#ifndef MX_MZERO

#define MX_MZERO  0x1
#define MX_WAITOK 0x2
#define MX_NOWAIT 0x4

#endif

#endif /* _mx_malloc_h_ */
