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

/* This MX kernel lib code was originally contributed by
 * Brice.Goglin@ens-lyon.org (LIP/INRIA/ENS-Lyon) */

#include "mal_auto_config.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include "mal_int.h"
#include "snf.h"

snf_inject_t hinj = NULL;

static int
klib_test_inject(void)
{

  int rc;
  static char buf[9000];

  rc = snf_inject_open(1, 0, &hinj);
  if (rc) {
    printk("Can't open injection handle on board 1: %d\n", rc);
    return rc;
  }

  if ((rc = snf_inject_send(hinj, 0, buf, 8400))) {
    printk("klib_test: unexpected return from snf_inject_send: %d\n", rc);
    return rc;
  }

  if ((rc = snf_inject_send(hinj, 0, buf, 60))) {
    printk("klib_test: unexpected return from snf_inject_send: %d\n", rc);
    return rc;
  }

  return 0;
}

static int
klib_test_capture(void)
{
  snf_handle_t hsnf;
  int rc, i;
  int num_rings = 8;
  snf_ring_t hrings[32];

  rc = snf_open(0, num_rings, NULL, 0, 0, &hsnf);
  if (rc) {
    printk("Can't open board 0 for receive: %d\n", rc);
    return rc;
  }

  for (i = 0; i < num_rings; i++) {
    rc = snf_ring_open_id(hsnf, i, &hrings[i]);
    if (rc) {
      printk("Can't open ring %d: %d\n", i, rc);
      break;
    }
  }

  printk("Successfully opened %d rings\n", num_rings);

  for (i = 0; i < num_rings; i++)
    snf_ring_close(hrings[i]);

  rc = snf_close(hsnf);
  if (rc) {
    printk("Can't close board 0 for receive\n");
    return rc;
  }

  return 0;
}

static int
klib_test_init_module(void)
{
  snf_init(SNF_VERSION_API);

  klib_test_capture();
  klib_test_inject();
  return 0;
}

static void
klib_test_cleanup_module(void)
{
  if (hinj)
    snf_inject_close(hinj);
}

module_init(klib_test_init_module);
module_exit(klib_test_cleanup_module);
