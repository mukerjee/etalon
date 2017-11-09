/*
 * Tuneable TCP Reno
 *
 * Matt Mukerjee <mukerjee@cs.cmu.edu>
 */

#include <linux/module.h>
#include <net/tcp.h>

static int ca_scale __read_mostly = 1;
static int ss_scale __read_mostly = 1;

module_param(ca_scale, int, 0644);
MODULE_PARM_DESC(ca_scale, "ca scale factor");
module_param(ss_scale, int, 0644);
MODULE_PARM_DESC(ss_scale, "ss scale factor");

/* Slow start is used when congestion window is no greater than the slow start
 * threshold. We base on RFC2581 and also handle stretch ACKs properly.
 * We do not implement RFC3465 Appropriate Byte Counting (ABC) per se but
 * something better;) a packet is only considered (s)acked in its entirety to
 * defend the ACK attacks described in the RFC. Slow start processes a stretch
 * ACK of degree N as if N acks of degree 1 are received back to back except
 * ABC caps N to 2. Slow start exits when cwnd grows over ssthresh and
 * returns the leftover acks to adjust cwnd in congestion avoidance mode.
 */
static u32 tune_tcp_slow_start(struct tcp_sock *tp, u32 acked)
{
  u32 cwnd = min(tp->snd_cwnd + acked, tp->snd_ssthresh);

  acked -= cwnd - tp->snd_cwnd;
  tp->snd_cwnd = min(cwnd, tp->snd_cwnd_clamp);

  return acked;
}

/* In theory this is tp->snd_cwnd += 1 / tp->snd_cwnd (or alternative w),
 * for every packet that was ACKed.
 */
static void tune_tcp_cong_avoid_ai(struct tcp_sock *tp, u32 w, u32 acked)
{
  /* If credits accumulated at a higher w, apply them gently now. */
  if (tp->snd_cwnd_cnt >= w) {
    tp->snd_cwnd_cnt = 0;
    tp->snd_cwnd++;
  }

  tp->snd_cwnd_cnt += acked;
  if (tp->snd_cwnd_cnt >= w) {
    u32 delta = tp->snd_cwnd_cnt / w;

    tp->snd_cwnd_cnt -= delta * w;
    tp->snd_cwnd += delta;
  }
  tp->snd_cwnd = min(tp->snd_cwnd, tp->snd_cwnd_clamp);
}

/*
 * TCP Reno congestion control
 * This is special case used for fallback as well.
 */
/* This is Jacobson's slow start and congestion avoidance.
 * SIGCOMM '88, p. 328.
 */
static void tcp_reno_tuner_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
  struct tcp_sock *tp = tcp_sk(sk);

  if (!tcp_is_cwnd_limited(sk))
    return;

  /* In "safe" area, increase. */
  /* if (tcp_in_slow_start(tp)) { */
  acked = tune_tcp_slow_start(tp, acked * ss_scale);
  /*   if (!acked) */
  /*     return; */
  /* } */
  /* In dangerous area, increase slowly. */
  /* tune_tcp_cong_avoid_ai(tp, tp->snd_cwnd, acked * ca_scale); */
}

/* Slow start threshold is half the congestion window (min 2) */
static u32 tcp_reno_tuner_ssthresh(struct sock *sk)
{
  const struct tcp_sock *tp = tcp_sk(sk);

  return max(tp->snd_cwnd >> 1U, 2U);
}

static struct tcp_congestion_ops tcp_ss __read_mostly = {
  .name         = "ss",
  .owner        = THIS_MODULE,
  .ssthresh     = tcp_reno_tuner_ssthresh,
  .cong_avoid   = tcp_reno_tuner_cong_avoid,
};

static int __init tcp_ss_register(void)
{
  return tcp_register_congestion_control(&tcp_ss);
}

static void __exit tcp_ss_unregister(void)
{
  tcp_unregister_congestion_control(&tcp_ss);
}

module_init(tcp_ss_register);
module_exit(tcp_ss_unregister);

MODULE_AUTHOR("Matt Mukerjee");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("TCP SS");
