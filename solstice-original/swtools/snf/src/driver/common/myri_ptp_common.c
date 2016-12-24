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
 * Copyright 2003 - 2010 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#include "mx_arch.h"
#include "mx_instance.h"
#include "mx_malloc.h"
#include "myri_ptp_common.h"

#if MYRI_ENABLE_PTP

/* the caller must be holding is->ptp_state_lock */
static
void
myri_ptp_reset(mx_instance_state_t *is)
{
  struct myri_ptp *ptp = is->ptp;
  struct myri_ptp_msg *m = NULL;
  struct myri_ptp_msg_head *q = NULL;
  struct myri_ptp_timestamp *t = NULL;
  struct myri_ptp_timestamp_head *tsq = NULL;

  if (ptp == NULL) {
    MX_WARN(("%s: %s: ptp is NULL\n", is->is_name, __func__));
    goto out;
  }

  q = &ptp->rx_msgq;
  while (!STAILQ_EMPTY(q)) {
    m = STAILQ_FIRST(q);
    STAILQ_REMOVE_HEAD(q, entries);
    myri_ptp_put_idle_msg_locked(ptp, m);
  }

  q = &ptp->rx_raw_msgq;
  while (!STAILQ_EMPTY(q)) {
    m = STAILQ_FIRST(q);
    STAILQ_REMOVE_HEAD(q, entries);
    myri_ptp_put_idle_msg_locked(ptp, m);
  }

  tsq = &ptp->tx_tsq;
  while (!STAILQ_EMPTY(tsq)) {
    t = STAILQ_FIRST(tsq);
    STAILQ_REMOVE_HEAD(tsq, entries);
    myri_ptp_put_idle_timestamp(ptp, t);
  }
out:
  return;
}

/* caller must be holding is->sync */
void
myri_ptp_fini(mx_instance_state_t *is)
{
  unsigned long flags;
  struct myri_ptp *ptp = is->ptp;

  flags = 0; /* -Wunused on non-linux */

  mx_spin_lock_irqsave(&is->ptp_state_lock, flags);
  is->ptp_state = MYRI_PTP_STOPPED;
  mx_spin_unlock_irqrestore(&is->ptp_state_lock, flags);

  mx_spin_lock_destroy(&is->ptp_state_lock);

  if (ptp == NULL) {
    MX_WARN(("%s: %s: ptp is NULL\n", is->is_name, __func__));
    return;
  }

  if (ptp->buffers != NULL)
    mx_kfree(ptp->buffers);

  if (ptp->rx_msgs != NULL)
    mx_kfree(ptp->rx_msgs);

  if (ptp->rx_raw_msgs != NULL)
    mx_kfree(ptp->rx_raw_msgs);

  if (ptp->tx_timestamps != NULL)
    mx_kfree(ptp->tx_timestamps);

  mx_kfree(ptp);
  is->ptp = NULL;

  return;
}

/* caller must be holding is->sync */
int
myri_ptp_alloc(mx_instance_state_t *is)
{
  int i;
  struct myri_ptp *p = NULL;
  struct myri_ptp_msg *m = NULL;
  struct myri_ptp_timestamp *t = NULL;

  p = mx_kmalloc(sizeof(*p), MX_WAITOK | MX_MZERO);
  if (p == NULL) {
    MX_WARN(("%s: %s: unable to kmalloc PTP struct\n", is->is_name, __func__));
    myri_ptp_fini(is);
    return ENOMEM;
  }

  /* alloc contiguous buffer for all rx_msg and rx_raw_msgs bufs */
  p->buffers = mx_kmalloc(2 * MYRI_PTP_NUM_MSGS * MYRI_PTP_MAX_MSG_LEN, MX_WAITOK | MX_MZERO);
  if (p->buffers == NULL) {
    MX_WARN(("%s: %s: unable to kmalloc PTP buffers\n", is->is_name, __func__));
    myri_ptp_fini(is);
    return ENOMEM;
  }

  STAILQ_INIT(&p->idle_rx_msgs);
  STAILQ_INIT(&p->rx_msgq);
  STAILQ_INIT(&p->idle_rx_raw_msgs);
  STAILQ_INIT(&p->rx_raw_msgq);
  STAILQ_INIT(&p->idle_tx_ts);
  STAILQ_INIT(&p->tx_tsq);

  /* allocate rx msgs */
  m = mx_kmalloc(MYRI_PTP_NUM_MSGS * sizeof(*m), MX_WAITOK|MX_MZERO);
  if (!m) {
    MX_WARN(("%s: unable to kmalloc() PTP rx msgs\n", is->is_name));
    myri_ptp_fini(is);
    return ENOMEM;
  }

  p->rx_msgs = m;
  p->rx_msgs_used = MYRI_PTP_NUM_MSGS;

  for (i = 0; i < MYRI_PTP_NUM_MSGS; i++) {
    /* we are not locked, but we have the only pointer */
    m[i].buf = &p->buffers[i * MYRI_PTP_MAX_MSG_LEN];
    myri_ptp_put_idle_msg_locked(p, &m[i]);
  }

  mal_assert(p->rx_msgs_used == 0);

  /* allocate rx raw msgs */
  m = mx_kmalloc(MYRI_PTP_NUM_MSGS * sizeof(*m), MX_WAITOK|MX_MZERO);
  if (!m) {
    MX_WARN(("%s: unable to kmalloc() PTP rx raw msgs\n", is->is_name));
    myri_ptp_fini(is);
    return ENOMEM;
  }

  p->rx_raw_msgs = m;
  p->rx_raw_msgs_used = MYRI_PTP_NUM_MSGS;

  for (i = 0; i < MYRI_PTP_NUM_MSGS; i++) {
    m[i].raw = 1;
    m[i].buf = &p->buffers[(MYRI_PTP_NUM_MSGS + i) * MYRI_PTP_MAX_MSG_LEN];
    /* we are not locked, but we have the only pointer */
    myri_ptp_put_idle_msg_locked(p, &m[i]);
  }
  mal_assert(p->rx_msgs_used == 0);

  /* allocate tx timestamps */
  t = mx_kmalloc(MYRI_PTP_NUM_MSGS * sizeof(*t), MX_WAITOK|MX_MZERO);
  if (!t) {
    MX_WARN(("%s: unable to kmalloc() PTP tx timestamps\n", is->is_name));
    myri_ptp_fini(is);
    return ENOMEM;
  }

  p->tx_timestamps = t;

  for (i = 0; i < MYRI_PTP_NUM_MSGS; i++)
    myri_ptp_put_idle_timestamp(p, &t[i]);

  mx_spin_lock_init(&is->ptp_state_lock, NULL, -1, "PTP spinlock");
  is->ptp_state = MYRI_PTP_RUNNING;
  is->ptp = p;

  return 0;
}

int
myri_ptp_store_rx_msg(mx_instance_state_t *is, myri_ptp_store_rx_msg_t *m, int is_kernel)
{
  unsigned long flags;
  struct myri_ptp *ptp = is->ptp;
  struct myri_ptp_msg *msg = NULL;
  struct myri_ptp_msg_head *rxq = m->raw ? &ptp->rx_raw_msgq : &ptp->rx_msgq;
  int status = ENODEV;

  flags = 0; /* -Wunused on non-linux */

  mx_spin_lock_irqsave(&is->ptp_state_lock, flags);

  if (is->ptp_state < MYRI_PTP_STARTING || ptp == NULL) {
    mx_spin_unlock_irqrestore(&is->ptp_state_lock, flags);
    return status;
  }

  msg = myri_ptp_get_idle_msg(is, m->raw);
  mx_spin_unlock_irqrestore(&is->ptp_state_lock, flags);
  if (msg != NULL) {
    msg->timestamp_ns = m->timestamp_ns;
    msg->length = m->msg_len;
    if (msg->length > MYRI_PTP_MAX_MSG_LEN) {
      MX_WARN(("%s: dropping incoming PTP msg with length %d\n", is->is_name, msg->length));
      status = ENOMEM;
      goto out;
    }
    status = mx_copyin((uaddr_t) m->msg_pointer, msg->buf, msg->length, is_kernel);
    if (status) {
      myri_ptp_put_idle_msg(is, msg);
      goto out;
    }
    mx_spin_lock_irqsave(&is->ptp_state_lock, flags);
    STAILQ_INSERT_TAIL(rxq, msg, entries);
    mx_spin_unlock_irqrestore(&is->ptp_state_lock, flags);
  } else {
    status = ENOMEM;
  }

out:
  return status;
}

static
int
myri_ptp_rx_avail(mx_instance_state_t *is, myri_ptp_rx_avail_t *x, int is_kernel)
{
  int status = ENODEV;
  unsigned long flags;
  struct myri_ptp *ptp = is->ptp;

  flags = 0; /* -Wunused on non-linux */

  mx_spin_lock_irqsave(&is->ptp_state_lock, flags);
  if (is->ptp_state < MYRI_PTP_STARTING || ptp == NULL)
    goto out_w_lock;

  x->avail = !STAILQ_EMPTY(&ptp->rx_msgq) || !STAILQ_EMPTY(&ptp->rx_raw_msgq);
  status = 0;

out_w_lock:
  mx_spin_unlock_irqrestore(&is->ptp_state_lock, flags);
  return status;
}

static
int
myri_ptp_start_stop(struct mx_instance_state *is, myri_ptp_start_stop_t *x, int is_kernel)
{
  int status = ENODEV;
  unsigned long flags;

  flags = 0; /* -Wunused on non-linux */

  mx_spin_lock_irqsave(&is->ptp_state_lock, flags);
  if (is->ptp_state < MYRI_PTP_STARTING && x->start_stop == MYRI_PTP_STOP)
    goto out_w_lock;

  if (x->start_stop == MYRI_PTP_START) {
    myri_ptp_reset(is);
    is->ptp_state = MYRI_PTP_RUNNING;
    status = 0;
  } else if (x->start_stop == MYRI_PTP_STOP) {
    is->ptp_state = MYRI_PTP_STOPPED;
    status = 0;
  } else {
    status = EINVAL;
  }

out_w_lock:
  mx_spin_unlock_irqrestore(&is->ptp_state_lock, flags);

  return status;
}

static
int
myri_ptp_get_rx_msg(mx_instance_state_t *is, myri_ptp_get_rx_msg_t *m, int is_kernel)
{
  unsigned long flags;
  struct myri_ptp *ptp = is->ptp;
  struct myri_ptp_msg_head *rxq = m->raw ? &ptp->rx_raw_msgq : &ptp->rx_msgq;
  struct myri_ptp_msg *msg = NULL;
  int status = ENODEV;

  flags = 0; /* -Wunused on non-linux */

  mx_spin_lock_irqsave(&is->ptp_state_lock, flags);

  if (is->ptp_state < MYRI_PTP_STARTING || ptp == NULL) {
    mx_spin_unlock_irqrestore(&is->ptp_state_lock, flags);
    return status;
  }

  msg = STAILQ_FIRST(rxq);
  if (msg != NULL)
    STAILQ_REMOVE_HEAD(rxq, entries);
  mx_spin_unlock_irqrestore(&is->ptp_state_lock, flags);

  if (msg != NULL) {
    m->timestamp_ns = msg->timestamp_ns;
    m->msg_len = msg->length;
    status = mx_copyout(msg->buf, (uaddr_t) m->msg_pointer, m->msg_len, is_kernel);
    myri_ptp_put_idle_msg(is, msg);
  } else {
    status = EAGAIN;
    m->msg_len = 0;
  }

  return status;
}

int
myri_ptp_put_tx_timestamp(mx_instance_state_t *is, myri_ptp_tx_timestamp_t *x, int is_kernel)
{
  unsigned long flags;
  struct myri_ptp *ptp = is->ptp;
  struct myri_ptp_timestamp *timestamp = NULL;
  int status = ENODEV;

  flags = 0; /* -Wunused on non-linux */

  mx_spin_lock_irqsave(&is->ptp_state_lock, flags);

  if (is->ptp_state < MYRI_PTP_STARTING || ptp == NULL) {
    mx_spin_unlock_irqrestore(&is->ptp_state_lock, flags);
    return status;
  }

  timestamp = myri_ptp_get_idle_timestamp(is);
  if (timestamp) {
    timestamp->ptp_type = x->ptp_type;
    timestamp->seq_id = x->seq_id;
    timestamp->timestamp_ns = x->timestamp_ns;
    status = 0;
    STAILQ_INSERT_TAIL(&ptp->tx_tsq, timestamp, entries);
  } else {
    status = ENOMEM;
  }

  mx_spin_unlock_irqrestore(&is->ptp_state_lock, flags);

  return status;
}

static
int
myri_ptp_get_tx_timestamp(mx_instance_state_t *is, myri_ptp_tx_timestamp_t *x, int is_kernel)
{
  unsigned long flags;
  struct myri_ptp *ptp = is->ptp;
  struct myri_ptp_timestamp *timestamp = NULL;
  struct myri_ptp_timestamp_head *q = NULL;
  struct myri_ptp_timestamp_head *idle = NULL;
  int status = ENODEV;

  flags = 0; /* -Wunused on non-linux */

  mx_spin_lock_irqsave(&is->ptp_state_lock, flags);

  if (is->ptp_state < MYRI_PTP_STARTING || ptp == NULL) {
    mx_spin_unlock_irqrestore(&is->ptp_state_lock, flags);
    return status;
  }

  q = &ptp->tx_tsq;
  idle = &ptp->idle_tx_ts;
  status = EAGAIN;
  x->timestamp_ns = 0ULL;

  STAILQ_FOREACH(timestamp, q, entries) {
    if (timestamp->ptp_type == x->ptp_type &&
        timestamp->seq_id == x->seq_id) {
	    x->timestamp_ns = timestamp->timestamp_ns;
	    STAILQ_REMOVE(q, timestamp, myri_ptp_timestamp, entries);
	    myri_ptp_put_idle_timestamp(ptp, timestamp);
	    status = 0;
	    break;
    }
  }

  mx_spin_unlock_irqrestore(&is->ptp_state_lock, flags);

  return status;
}

static
int
myri_ptp_check_instance(mx_instance_state_t *is, int check_dead)
{
  if (!is)
    return ENODEV;
  if (check_dead && mx_is_dead(is)) {
    /* if the board is dead, return an error to the application...
     * except in case of parity since the recovery is notified to the
     * application through the wait_status or get_board_status
     * (other ioctls can still be processed safely since they do not
     * touch the board for anything that won't be redone a afterwards)
     */
    if (!(is->flags & MX_PARITY_RECOVERY)) {
      MX_WARN(("%s: firmware dead on board %d, ignoring ioctl\n", is->is_name, is->id));
      return EIO;
    }
  }
  return 0;
}

int
myri_ptp_ioctl(uint32_t cmd, const uaddr_t in, uint32_t is_kernel)
{
  int status = 0;
  mx_instance_state_t *is = NULL;
  uaddr_t out = in;

  switch (cmd) {
  default:
    MX_WARN(("%s: unknown PTP ioctl(%d)\n", is->is_name, cmd));
    break;
  case MYRI_PTP_STORE_RX_MSG:
    {
      myri_ptp_store_rx_msg_t x;

      if ((status = mx_copyin(in, &x, sizeof(x), is_kernel)))
        break;

      is = mx_get_instance(x.board);
      status = myri_ptp_check_instance(is, 0);
      if (status)
	break;

      status = myri_ptp_store_rx_msg(is, &x, is_kernel);
    }
    break;
  case MYRI_PTP_RX_AVAIL:
    {
      myri_ptp_rx_avail_t x;
      if ((status = mx_copyin(in, &x, sizeof(x), is_kernel)))
        break;

      is = mx_get_instance(x.board);
      status = myri_ptp_check_instance(is, 0);
      if (status)
	break;

      if ((status = myri_ptp_rx_avail(is, &x, is_kernel)))
        break;
      status = mx_copyout(&x, out, sizeof(x), is_kernel);
    }
    break;
  case MYRI_PTP_GET_RX_MSG:
    {
      myri_ptp_get_rx_msg_t x;
      if ((status = mx_copyin(in, &x, sizeof(x), is_kernel)))
        break;

      is = mx_get_instance(x.board);
      status = myri_ptp_check_instance(is, 0);
      if (status)
	break;

      if ((status = myri_ptp_get_rx_msg(is, &x, is_kernel)))
        break;
      status = mx_copyout(&x, out, sizeof(x), is_kernel);
    }
    break;
  case MYRI_PTP_START_STOP:
    {
      myri_ptp_start_stop_t x;
      if ((status = mx_copyin(in, &x, sizeof(x), is_kernel)))
        break;

      is = mx_get_instance(x.board);
      status = myri_ptp_check_instance(is, 0);
      if (status)
	break;

      status = myri_ptp_start_stop(is, &x, is_kernel);
    }
    break;
  case MYRI_PTP_PUT_TX_TIMESTAMP:
    {
      myri_ptp_tx_timestamp_t x;

      if ((status = mx_copyin(in, &x, sizeof(x), is_kernel)))
        break;

      is = mx_get_instance(x.board);
      status = myri_ptp_check_instance(is, 0);
      if (status)
	break;

      status = myri_ptp_put_tx_timestamp(is, &x, is_kernel);
    }
    break;
  case MYRI_PTP_GET_TX_TIMESTAMP:
    {
      myri_ptp_tx_timestamp_t x;

      if ((status = mx_copyin(in, &x, sizeof(x), is_kernel)))
        break;

      is = mx_get_instance(x.board);
      status = myri_ptp_check_instance(is, 0);
      if (status)
	break;

      if ((status = myri_ptp_get_tx_timestamp(is, &x, is_kernel)))
        break;
      status = mx_copyout(&x, out, sizeof(x), is_kernel);
    }
    break;
  }

  if (is)
	  mx_release_instance(is);

  return status;
}
#endif /* MYRI_ENABLE_PTP */
