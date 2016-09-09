#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/ip.h>

#undef __KERNEL__
#include <linux/netfilter_ipv4.h>
#define __KERNEL__

#define IP_HDR_LEN 20
#define UDP_HDR_LEN 8
#define APP_HDR_LEN 0
#define APP_PORT 5001

#define DEBUG 0

void remove_data_from_packet(struct sk_buff *skb, unsigned char proto) {
    struct iphdr *iph;
    
    struct tcphdr *tcph;
    struct udphdr *udph;

    unsigned short data_len;
    int data_len_s;
    unsigned int trans_hdr_len = 0;

    __u16 dst_port = 0;

    if (proto == IPPROTO_TCP) {
        tcph = (struct tcphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);
        dst_port = ntohs(tcph->dest);
        trans_hdr_len = tcph->doff * 4;
    } else if (proto == IPPROTO_UDP) {
        udph = (struct udphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);
        dst_port = ntohs(udph->dest);
        trans_hdr_len = UDP_HDR_LEN;
    }
    if (dst_port == APP_PORT) {
        data_len_s = (int)skb->len - (int)IP_HDR_LEN
            - (int)trans_hdr_len - (int)APP_HDR_LEN;
        if (data_len_s < (int)sizeof(data_len)) {
            if (DEBUG)
                printk(KERN_INFO "short packet: %d\n", skb->len);
            return; // e.g., SYN, APP Control
        }

        data_len = skb->len;


        skb_linearize(skb);
              
        skb->len = IP_HDR_LEN + trans_hdr_len + APP_HDR_LEN + sizeof(data_len);
        skb_set_tail_pointer(skb, skb->len);

        if (DEBUG)
            printk(KERN_INFO "old len: %d, new len: %d\n", data_len,
                   skb->len);

        // put the original data len into last two bytes of app payload
        *(unsigned short *)(skb_tail_pointer(skb) - sizeof(data_len)) =
            htons(data_len);

        iph = (struct iphdr *) skb_header_pointer(skb, 0, 0, NULL);

        iph->tos = 1; // mark as removed

        // Update len
        iph->tot_len = htons(skb->len);

        // IP checksum
        iph->check = 0;
        ip_send_check(iph);


        // Transport checksum (and len)
        if (proto == IPPROTO_TCP) {
            tcph = (struct tcphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);
            tcph->check = 0;
            tcph->check = ~csum_tcpudp_magic(iph->saddr, iph->daddr,
                                             skb->len - IP_HDR_LEN,
                                             IPPROTO_TCP, 0);
        } else if (proto == IPPROTO_UDP) {
            udph = (struct udphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);
            udph->len = htons(skb->len - IP_HDR_LEN);
            udph->check = 0;
            udph->check = ~csum_tcpudp_magic(iph->saddr, iph->daddr,
                                             skb->len - IP_HDR_LEN,
                                             IPPROTO_UDP, 0);
        }
    }
}

unsigned int pkt_remove_begin(void *priv,
                              struct sk_buff *skb,
                              const struct nf_hook_state *state) { 
    struct iphdr *iph;
    if (skb) {
        iph = (struct iphdr *) skb_header_pointer(skb, 0, 0, NULL);

        if (iph)
            remove_data_from_packet(skb, iph->protocol);
    }
    return NF_ACCEPT;
}

static struct nf_hook_ops pkt_remove_ops;

int init_module(void) {
    pkt_remove_ops.hook = pkt_remove_begin;
    pkt_remove_ops.hooknum = NF_IP_POST_ROUTING;
    pkt_remove_ops.pf = PF_INET;
    pkt_remove_ops.priority = NF_IP_PRI_FIRST;

    printk(KERN_ALERT "pkt remove started ...\n");
    return nf_register_hook(&pkt_remove_ops);
}

void cleanup_module(void) {
    nf_unregister_hook(&pkt_remove_ops);
    printk(KERN_ALERT "pkt remove stopped ...\n");
} 

MODULE_LICENSE("GPL");
