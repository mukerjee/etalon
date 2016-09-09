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
#define APP_PORT 5001

#define DEBUG 0

void add_data_to_packet(struct sk_buff *skb, unsigned char proto) {
    struct iphdr *iph;
    
    struct tcphdr *tcph;
    struct udphdr *udph;

    unsigned short data_len;
    int extra_space_needed;

    __u16 dst_port = 0;

    if (proto == IPPROTO_TCP) {
        tcph = (struct tcphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);
        dst_port = ntohs(tcph->dest);
    } else if (proto == IPPROTO_UDP) {
        udph = (struct udphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);
        dst_port = ntohs(udph->dest);
    }

    if (dst_port == APP_PORT) {
        skb_linearize(skb);

        // what data len should be
        data_len = ntohs(*(unsigned short *)(skb_tail_pointer(skb)
                                             - sizeof(data_len)));


        if (DEBUG)
            printk(KERN_INFO "target size = %d\n", data_len);

        extra_space_needed = (int)data_len - (int)skb->len
            - (int)skb_tailroom(skb);

        if (extra_space_needed  > 0) {
            if (pskb_expand_head(skb, 0, extra_space_needed, GFP_ATOMIC)) {
                // allocation failed
                // for now just let the packet go out
                if (DEBUG)
                    printk(KERN_INFO "couldn't allocate data\n");

                return;
            }
            if (DEBUG)
                printk(KERN_INFO "allocation success tailroom=%d\n",
                       skb_tailroom(skb));
                  
            // allocation success
            skb_linearize(skb);
        }

        // add the amount of data the packet told us to add.
        if ((int)data_len - (int)skb->len > 0)
            skb_put(skb, data_len - skb->len);

        iph = (struct iphdr *) skb_header_pointer(skb, 0, 0, NULL);

        if (DEBUG)
            printk(KERN_INFO "ip tot should be %d\n", data_len);

        // Manipulating necessary header fields
        iph->tot_len = htons(skb->len);

        if (DEBUG)
            printk(KERN_INFO "ip tot_len is %d\n", ntohs(iph->tot_len));

        if (proto == IPPROTO_UDP) {
            udph = (struct udphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);
            udph->len = htons(skb->len - IP_HDR_LEN);
            if (DEBUG)
                printk(KERN_INFO "udp len is %d\n", ntohs(udph->len));
        }
    }
}

unsigned int pkt_add_begin(void *priv,
                           struct sk_buff *skb,
                           const struct nf_hook_state *state) { 
    struct iphdr *iph;

    if (skb) {
        iph = (struct iphdr *) skb_header_pointer(skb, 0, 0, NULL);

        if (iph && iph->tos == 1)
            add_data_to_packet(skb, iph->protocol);
    }
    return NF_ACCEPT;
}

static struct nf_hook_ops pkt_add_ops;

int init_module(void) {
    pkt_add_ops.hook = pkt_add_begin;
    pkt_add_ops.hooknum = NF_IP_PRE_ROUTING;
    pkt_add_ops.pf = PF_INET;
    pkt_add_ops.priority = NF_IP_PRI_FIRST;

    printk(KERN_ALERT "add data started ...\n");
    return nf_register_hook(&pkt_add_ops);
}

void cleanup_module(void) {
    nf_unregister_hook(&pkt_add_ops);
    printk(KERN_ALERT "add data stopped ...\n");
} 

MODULE_LICENSE("GPL");
