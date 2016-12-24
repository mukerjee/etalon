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
 * Copyright 2009 by Myricom, Inc.  All rights reserved.                 *
 *************************************************************************/
#ifndef _myri_clksync_h
#define _myri_clksync_h

/*
 * Converting between two clocks in nanoseconds, in this case from a NIC clock
 * to the host clock.  The expected usage is for a background process to
 * periodically update the clockparams.
 */
#define MYRI_CLKSYNC_MULTIPLIER_SHIFT 30
typedef struct {
  uint64_t nsecs_base;
  int64_t  hostnic_drift_nsecs;
  int64_t  multiplier;
  int64_t  error_nsecs;
} myri_clksync_params_t;

typedef struct {
  uint32_t	     seqnum;
  myri_clksync_params_t clkp;
} myri_clksync_seq_t;


/* Accumulate ticks into nanoseconds.
 * There's no function to initialize the myri_clksync_nticks_t state, its up to the
 * user to initialize nsecs to either be representative of system host time or
 * just 0 if the nsecs are to be monotonic from 0.
 */
typedef struct {
  uint64_t  ticks;
  uint64_t  ticks_mask;
  uint64_t  nsecs;
  uint64_t  nsecs_per_tick;
} myri_clksync_nticks_t;

static inline uint64_t
myri_clksync_nticks_update(myri_clksync_nticks_t *ntss, uint64_t ticks)
{
  uint64_t delta_ticks;
  uint64_t nsecs;

  delta_ticks = (ticks - ntss->ticks) & ntss->ticks_mask;
  nsecs = delta_ticks * ntss->nsecs_per_tick;
  nsecs += ntss->nsecs;
  ntss->ticks = ticks;
  ntss->nsecs = nsecs;
  return nsecs;
}

static inline uint64_t
myri_clksync_nticks_to_nsecs(const myri_clksync_nticks_t *ntss, uint64_t ticks)
{
  uint64_t delta_ticks;
  uint64_t nsecs;
  
  delta_ticks = (ticks - ntss->ticks) & ntss->ticks_mask;
  if (delta_ticks > ntss->ticks_mask/2) {
    delta_ticks = (ntss->ticks - ticks) & ntss->ticks_mask;
    nsecs = ntss->nsecs - delta_ticks * ntss->nsecs_per_tick;
  }
  else
    nsecs = ntss->nsecs + delta_ticks * ntss->nsecs_per_tick;
  return nsecs;
}

static inline uint64_t
__myri_clksync_convert_nsecs(volatile myri_clksync_params_t *clkp, uint64_t nsecs_in)
{
  uint64_t nsecs = nsecs_in + clkp->hostnic_drift_nsecs;
  nsecs += ((int64_t)(nsecs_in - clkp->nsecs_base)
               * clkp->multiplier) >> MYRI_CLKSYNC_MULTIPLIER_SHIFT;
  return nsecs;
}

#include "mal_stbar.h"

static inline uint64_t
myri_clksync_convert_nsecs(volatile myri_clksync_seq_t *clksync, uint64_t nsecs_in)
{
  uint64_t nsecs = 0;
  uint32_t seqnum;

  do {
try_again:
    seqnum = clksync->seqnum;
    MAL_READBAR();

    if (unlikely(seqnum & 1)) { /* back off, hit middle of update */
      MAL_CPU_RELAX();
      goto try_again;
    }
    nsecs = __myri_clksync_convert_nsecs(&clksync->clkp, nsecs_in);
    MAL_READBAR();
  } while (seqnum != clksync->seqnum);

  return nsecs;
}
#endif /* _myri_clksync_h */
