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
  if (tcp_in_slow_start(tp)) {
    acked = tcp_slow_start(tp, acked * ss_scale);
    if (!acked)
      return;
  }
  /* In dangerous area, increase slowly. */
  tcp_cong_avoid_ai(tp, tp->snd_cwnd, acked * ca_scale);
}

static struct tcp_congestion_ops tcp_reno_tuner __read_mostly = {
  .name         = "reno-tuner",
  .owner        = THIS_MODULE,
  .ssthresh     = tcp_reno_ssthresh,
  .cong_avoid   = tcp_reno_tuner_cong_avoid,
};

static int __init tcp_reno_tuner_register(void)
{
  return tcp_register_congestion_control(&tcp_reno_tuner);
}

static void __exit tcp_reno_tuner_unregister(void)
{
  tcp_unregister_congestion_control(&tcp_reno_tuner);
}

module_init(tcp_reno_tuner_register);
module_exit(tcp_reno_tuner_unregister);

MODULE_AUTHOR("Matt Mukerjee");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("TCP Reno Tuner");
