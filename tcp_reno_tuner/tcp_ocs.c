/*
 * TCP OCS
 *
 * Matt Mukerjee <mukerjee@cs.cmu.edu>
 */

#include <linux/module.h>
#include <net/tcp.h>

static int ca_scale __read_mostly = 1;
static int ss_scale __read_mostly = 1;

static int circuit_cwnd = 32;

struct tcp_ocs {
  u32 have_circuit;
  u32 old_cwnd;
};

module_param(ca_scale, int, 0644);
MODULE_PARM_DESC(ca_scale, "ca scale factor");
module_param(ss_scale, int, 0644);
MODULE_PARM_DESC(ss_scale, "ss scale factor");
module_param(circuit_cwnd, int, 0644);
MODULE_PARM_DESC(circuit_cwnd, "circuit cwnd");

static void tcp_ocs_init(struct sock *sk)
{
  struct tcp_ocs *ca = inet_csk_ca(sk);

  ca->have_circuit = 0;
  ca->old_cwnd = 1;
}

static void tcp_ocs_ce_state_0_to_1(struct sock *sk) {
  struct tcp_ocs *ca = inet_csk_ca(sk);
  struct tcp_sock *tp = tcp_sk(sk);

  if (!ca->have_circuit)
    ca->old_cwnd = tp->snd_cwnd;

  tp->snd_cwnd = circuit_cwnd;
  ca->have_circuit = 1;
}

static void tcp_ocs_ce_state_1_to_0(struct sock *sk) {
  struct tcp_ocs *ca = inet_csk_ca(sk);
  struct tcp_sock *tp = tcp_sk(sk);

  if (ca->have_circuit)
    tp->snd_cwnd = ca->old_cwnd;
  
  ca->have_circuit = 0;
}

static void tcp_ocs_cwnd_event(struct sock *sk, enum tcp_ca_event ev)
{
  switch (ev) {
  case CA_EVENT_ECN_IS_CE:
    tcp_ocs_ce_state_0_to_1(sk);
    break;
  case CA_EVENT_ECN_NO_CE:
    tcp_ocs_ce_state_1_to_0(sk);
    break;
  default:
    break;
  }
}

static void tcp_ocs_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
  const struct tcp_ocs *ca = inet_csk_ca(sk);
  struct tcp_sock *tp = tcp_sk(sk);

  if (ca->have_circuit) {
    tp->snd_cwnd = circuit_cwnd;
  } else {
    tcp_reno_cong_avoid(sk, ack, acked);
  }
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
