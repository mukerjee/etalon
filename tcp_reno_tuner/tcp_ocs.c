/*
 * TCP OCS
 *
 * Matt Mukerjee <mukerjee@cs.cmu.edu>
 */

#include <linux/module.h>
#include <net/tcp.h>

struct tcp_ocs {
  u32 have_circuit;
};

static void tcp_ocs_init(struct sock *sk)
{
  struct tcp_ocs *ca = inet_csk_ca(sk);
  ca -> have_circuit = 0;
}

static void tcp_ocs_ce(struct sock *sk) {
  struct tcp_ocs *ca = inet_csk_ca(sk);
  struct tcp_sock *tp = tcp_sk(sk);
  if (!ca->have_circuit)
    tp->snd_cwnd *= 8;
  ca->have_circuit = 1;
}

static void tcp_ocs_no_ce(struct sock *sk) {
  struct tcp_ocs *ca = inet_csk_ca(sk);
  struct tcp_sock *tp = tcp_sk(sk);
  if (ca->have_circuit)
    tp->snd_cwnd /= 8;
  ca->have_circuit = 0;
}

static void tcp_ocs_cwnd_event(struct sock *sk, enum tcp_ca_event ev)
{
  switch (ev) {
  case CA_EVENT_ECN_IS_CE:
    tcp_ocs_ce(sk);
    break;
  case CA_EVENT_ECN_NO_CE:
    tcp_ocs_no_ce(sk);
    break;
  default:
    break;
  }
}

static void tcp_ocs_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
  const struct tcp_ocs *ca = inet_csk_ca(sk);
  int scale = ca->have_circuit ? 8 : 1;
  tcp_reno_cong_avoid(sk, ack, acked * scale);
}

static struct tcp_congestion_ops tcp_ocs __read_mostly = {
  .name         = "ocs",
  .owner        = THIS_MODULE,
  .init         = tcp_ocs_init,
  .cwnd_event   = tcp_ocs_cwnd_event,
  .ssthresh     = tcp_reno_ssthresh,
  .cong_avoid   = tcp_ocs_cong_avoid,
};

static int __init tcp_ocs_register(void)
{
  return tcp_register_congestion_control(&tcp_ocs);
}

static void __exit tcp_ocs_unregister(void)
{
  tcp_unregister_congestion_control(&tcp_ocs);
}

module_init(tcp_ocs_register);
module_exit(tcp_ocs_unregister);

MODULE_AUTHOR("Matt Mukerjee");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("TCP OCS");
