/*
 * reTCP
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

struct retcp {
  u32 have_circuit;
  u32 jumped;
};

static void retcp_init(struct sock *sk)
{
  struct retcp *ca = inet_csk_ca(sk);
  ca->have_circuit = 0;
  ca->jumped = 0;
}

static void retcp_in_ack(struct sock *sk, u32 flags)
{
  struct retcp *ca = inet_csk_ca(sk);
  ca->have_circuit = flags & CA_ACK_ECE;
}

static void retcp_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
  int old_cwnd;
  struct retcp *ca = inet_csk_ca(sk);
  struct tcp_sock *tp = tcp_sk(sk);
  tcp_reno_cong_avoid(sk, ack, acked);

  if (ca->have_circuit && !ca->jumped) {
    old_cwnd = tp->snd_cwnd;
    tp->snd_cwnd *= jump_up;
    ca->jumped = 1;
    printk(KERN_WARNING "reTCP increase, old cwnd: %d, new cwnd: %d, ack: %u\n",
	   old_cwnd, tp->snd_cwnd, ack);
  }
  if (!ca->have_circuit && ca->jumped) {
    old_cwnd = tp->snd_cwnd;
    tp->snd_cwnd /= jump_down;
    ca->jumped = 0;
    printk(KERN_WARNING "reTCP decrease, old cwnd: %d, new cwnd: %d, ack: %u\n",
	   old_cwnd, tp->snd_cwnd, ack);
  }
}

static struct tcp_congestion_ops retcp __read_mostly = {
  .name         = "retcp",
  .owner        = THIS_MODULE,
  .init         = retcp_init,
  .in_ack_event = retcp_in_ack,
  .ssthresh     = tcp_reno_ssthresh,
  .cong_avoid   = retcp_cong_avoid,
  .undo_cwnd    = tcp_reno_undo_cwnd,
};

static int __init retcp_register(void)
{
  return tcp_register_congestion_control(&retcp);
}

static void __exit retcp_unregister(void)
{
  tcp_unregister_congestion_control(&retcp);
}

module_init(retcp_register);
module_exit(retcp_unregister);

MODULE_AUTHOR("Matt Mukerjee");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("reTCP");
