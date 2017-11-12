/*
 * Tuneable TCP Reno
 *
 * Matt Mukerjee <mukerjee@cs.cmu.edu>
 */

#include <linux/module.h>
#include <net/tcp.h>

static void tcp_ss_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
  struct tcp_sock *tp = tcp_sk(sk);
  tp->snd_cwnd += acked;
}

static struct tcp_congestion_ops tcp_ss __read_mostly = {
  .name         = "ss",
  .owner        = THIS_MODULE,
  .ssthresh     = tcp_reno_ssthresh,
  .cong_avoid   = tcp_ss_cong_avoid,
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
