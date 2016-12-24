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

#include "mal_auto_config.h"
#include "mal_stbar.h"

#ifdef __MAL_STBAR 
/* Emit a store barrier. */
void 
mx__stbar(void)
{
  __MAL_STBAR ();
  return;
}
#endif

/* Emit a read barrier. */
#ifdef __MAL_READBAR
void 
mx__readbar(void)
{
  __MAL_READBAR ();
  return;
}
#endif

/* Emit a write barrier. */
#ifdef __MAL_STBAR
void 
mx__writebar(void)
{
  __MAL_STBAR ();
  return;
}
#endif
