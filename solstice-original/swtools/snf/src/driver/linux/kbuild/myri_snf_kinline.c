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
 * Copyright 2010 by Myricom, Inc.  All rights reserved.                 *
 *************************************************************************/

/*
 * This is an example application that internally uses the Sniffer kernel
 * library for acquiring packets and reinjecting them on the same or another
 * device.
 */

#include "mal.h"
#include "mal_thread.h"
#include "mx_klib_test.h"
#include "snf_inline.h"

int board_src = 0;
int board_dst = 1;
int rings = 4;
int uni = 0;
static int inls_started = 0;
module_param(board_src, int, S_IRUGO);
module_param(board_dst, int, S_IRUGO);
module_param(uni, int, S_IRUGO);
module_param(rings, int, S_IRUGO);

static int
kinl_test_init_module(void)
{
  int num_dev;

  inl_init(NULL);
  if (inl_num_devices(&num_dev) || num_dev == 0) {
    MAL_PRINT(("Could not find any myri SNF devices\n"));
    return -1;
  }

  /* Assume 4 rings and cpumask 0xaa for first, 0x55 for second */
  if ((inl_add(board_src, board_dst, rings, 0xaa)))
    return -1;
  if (uni == 0)
    if ((inl_add(board_dst, board_src, rings, 0x55)))
      return -1;

  if (inl_start()) {
    MAL_PRINT(("Problem starting some inls\n"));
  }
  else {
    inls_started = 1;
    MAL_PRINT(("Started inline devices\n"));
  }
  return 0;
}

static void
kinl_test_cleanup_module(void)
{
  if (inls_started) {
    MAL_PRINT(("Stopping inline devices\n"));
    inl_stop();
    inl_wait();
    inl_dump_stats();
  }
}

module_init(kinl_test_init_module);
module_exit(kinl_test_cleanup_module);
