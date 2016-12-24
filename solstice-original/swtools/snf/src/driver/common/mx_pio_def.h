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

/* This file implements defines MX_PIO_READ and MX_PIO_WRITE 
   based on UDRV or not. */

#ifndef _mx_pio_def_h_
#define _mx_pio_def_h_

#include "mal_stbar.h"

#if MAL_OS_UDRV

#include "uba_sim.h"

#define MX_PIO_READ(addr)					\
  (mx_lxgdb ? \
   ({								\
    typeof (*(addr)) _tmp;					\
    fake_pci_pio_read((void *) (addr), &_tmp, sizeof(*addr),	\
		       __FILE__, __LINE__);			\
    _tmp;}) : *(addr))

#define MX_PIO_WRITE(addr, val)					\
  do {								\
    typeof (*(addr)) _tmp = val;				\
    if (mx_lxgdb)						\
       fake_pci_pio_write((char *) addr, &_tmp, sizeof(*addr),	\
		       __FILE__, __LINE__);			\
    else								\
       *(addr) = _tmp;						\
								\
  } while (0);
#else
#define MX_PIO_READ(addr) (*(addr))
#define MX_PIO_WRITE(addr, val) (*(addr)) = val
#endif

#endif /* _mx_pio_def_h_ */
