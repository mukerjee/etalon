/*
 * TCP OCS
 *
 * Matt Mukerjee <mukerjee@cs.cmu.edu>
 */

#include <linux/module.h>
#include <net/tcp.h>

static int jump_up __read_mostly = 2;
static int jump_down __read_mostly = 2;

module_param(jump_up, int, 0644);
MODULE_PARM_DESC(jump_up, "CA jump up when given circuit");
module_param(jump_down, int, 0644);
MODULE_PARM_DESC(jump_up, "CA jump down when loses circuit");

struct tcp_ocs {
  u32 have_circuit;
  u32 jumped;
};

static void tcp_ocs_init(struct sock *sk)
{
  struct tcp_ocs *ca = inet_csk_ca(sk);
  ca->have_circuit = 0;
  ca->jumped = 0;
}

static void tcp_ocs_in_ack(struct sock *sk, u32 flags)
{
  struct tcp_ocs *ca = inet_csk_ca(sk);
  ca->have_circuit = flags & CA_ACK_ECE;
}

static void tcp_ocs_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
  struct tcp_ocs *ca = inet_csk_ca(sk);
  struct tcp_sock *tp = tcp_sk(sk);
  tcp_reno_cong_avoid(sk, ack, acked);

  if (ca->have_circuit && !ca->jumped) {
    tp->snd_cwnd *= jump_up;
    ca->jumped = 1;
  }
  if (!ca->have_circuit && ca->jumped) {
    tp->snd_cwnd /= jump_down;
    ca->jumped = 0;
  }
}

static struct tcp_congestion_ops tcp_ocs __read_mostly = {
  .name         = "ocs",
  .owner        = THIS_MODULE,
  .init         = tcp_ocs_init,
  .in_ack_event = tcp_ocs_in_ack,
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
