#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/ip.h>
#include <net/pkt_sched.h>

#undef __KERNEL__
#include <linux/netfilter_ipv4.h>
#define __KERNEL__

#define IP_HDR_LEN 20
#define APP_PORT 5001

#define DEBUG 0

//unsigned int test = 0;

unsigned int pkt_sched_begin(void *priv,
                             struct sk_buff *skb,
                             const struct nf_hook_state *state) { 
    struct iphdr *iph;
    struct tcphdr *tcph;
    __u16 dst_port = 0;
    struct net_device *dev;
    struct Qdisc *qd;

    if (skb) {
        iph = (struct iphdr *) skb_header_pointer(skb, 0, 0, NULL);

        if (iph && iph->protocol == IPPROTO_TCP) {
            tcph = (struct tcphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);
            dst_port = ntohs(tcph->dest);
            if (dst_port == APP_PORT) {
                skb->mark = 1;
                //printk(KERN_INFO "test=%d\n", test);
                //test++;
                dev = skb->dev;
                qd = qdisc_lookup(dev, TC_H_MAKE(1, 1));
                printk(KERN_INFO "qd = %p\n", qd);
            }
        }
    }
    return NF_ACCEPT;
}

static struct nf_hook_ops pkt_sched_ops;

int init_module(void) {
    pkt_sched_ops.hook = pkt_sched_begin;
    pkt_sched_ops.hooknum = NF_IP_PRE_ROUTING;
    pkt_sched_ops.pf = PF_INET;
    pkt_sched_ops.priority = NF_IP_PRI_FIRST;

    printk(KERN_ALERT "packet schedule started ...\n");
    return nf_register_hook(&pkt_sched_ops);
}

void cleanup_module(void) {
    nf_unregister_hook(&pkt_sched_ops);
    printk(KERN_ALERT "packet schedule stopped ...\n");
} 

MODULE_LICENSE("GPL");
