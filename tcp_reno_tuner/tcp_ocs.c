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
  ca->have_circuit = 0;
}

static void tcp_ocs_ce(struct sock *sk) {
  struct tcp_ocs *ca = inet_csk_ca(sk);
  struct tcp_sock *tp = tcp_sk(sk);
  /* printk("tcp_ocs: entering ce %d\n", ca->have_circuit); */
  /* if (!ca->have_circuit) */
  /*   tp->snd_cwnd *= 8; */
  ca->have_circuit = 1;
  /* printk("tcp_ocs: exiting ce %d\n", ca->have_circuit); */
}

static void tcp_ocs_no_ce(struct sock *sk) {
  struct tcp_ocs *ca = inet_csk_ca(sk);
  struct tcp_sock *tp = tcp_sk(sk);
  /* printk("tcp_ocs: entering no_ce %d\n", ca->have_circuit); */
  /* if (ca->have_circuit) */
  /*   tp->snd_cwnd /= 8; */
  ca->have_circuit = 0;
  /* printk("tcp_ocs: exiting no_ce %d\n", ca->have_circuit); */
}

/* static void tcp_ocs_cwnd_event(struct sock *sk, enum tcp_ca_event ev) */
/* { */
/*   printk("tcp_ocs: entering cwnd_event\n"); */
/*   switch (ev) { */
/*   case CA_EVENT_ECN_IS_CE: */
/*     printk("tcp_ocs: entering CA_EVENT_ECN_IS_CE\n"); */
/*     tcp_ocs_ce(sk); */
/*     break; */
/*   case CA_EVENT_ECN_NO_CE: */
/*     printk("tcp_ocs: entering CA_EVENT_ECN_NO_CE\n"); */
/*     tcp_ocs_no_ce(sk); */
/*     break; */
/*   default: */
/*     break; */
/*   } */
/*   printk("tcp_ocs: exiting cwnd_event\n"); */
/* } */

static void tcp_ocs_in_ack(struct sock *sk, u32 flags) {
  /* struct tcp_ocs *ca = inet_csk_ca(sk); */
  /* printk("tcp_ocs: in ack flag: %d, ecn flags: %d\n", flags, tcp_sk(sk)->ecn_flags); */
  if (flags & CA_ACK_ECE) {
    /* printk("tcp_ocs: ece %d\n", ca->have_circuit); */
    tcp_ocs_ce(sk);
  } else {
    tcp_ocs_no_ce(sk);
  }
}

/* static u32 tcp_ocs_ssthresh(struct sock *sk) */
/* { */
/*   const struct tcp_ocs *ca = inet_csk_ca(sk); */
/*   const struct tcp_sock *tp = tcp_sk(sk); */
/*   int scale = ca->have_circuit ? 16 : 2; */

/*   return max(tp->snd_cwnd / scale, 2U); */
/* } */

static void tcp_ocs_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
  const struct tcp_ocs *ca = inet_csk_ca(sk);
  /* int scale = 1; */
  int scale = ca->have_circuit ? 8 : 1;
  /* printk("tcp_ocs: have circuit %d\n", ca->have_circuit); */
  /* if (scale == 8) { */
  /*   printk("tcp_ocs: circuit mode\n"); */
  /* } else { */
  /*   printk("tcp_ocs: packet mode\n");     */
  /* } */
  tcp_reno_cong_avoid(sk, ack, acked * scale);
}

static struct tcp_congestion_ops tcp_ocs __read_mostly = {
  .name         = "ocs",
  .owner        = THIS_MODULE,
  .init         = tcp_ocs_init,
  .in_ack_event = tcp_ocs_in_ack,
  /* .cwnd_event   = tcp_ocs_cwnd_event, */
  /* .ssthresh     = tcp_ocs_ssthresh, */
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
