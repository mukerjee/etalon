/*************************************************************************
 * The contents of this file are subject to the MYRICOM ABSTRACTION      *
 * LAYER (MAL) SOFTWARE AND DOCUMENTATION LICENSE (the "License").       *
 * User may not use this file except in compliance with the License.     *
 * The full text of the License can found in LICENSE.TXT                 *
 *                                                                       *
 * Software distributed under the License is distributed on an "AS IS"   *
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See  *
 * the License for the specific language governing rights and            *
 * limitations under the License.                                        *
 *                                                                       *
 * Copyright 2003 - 2009 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#include "mal.h"
#include "mal_io.h"

#ifdef MAL_KERNEL
#error Cannot compile mal_internal.c into kernel
#endif

#include "mal_fops.c"
#include "mal_stbar.c"
#include "mal_timing.c"
#include "mal_utils.c"
#include "mal_kutils.c"
#include "mal_thread.c"
#if MYRI_ENABLE_PTP
#include "myri_raw.c"
#endif

uint32_t mal_debug_mask = 0;
