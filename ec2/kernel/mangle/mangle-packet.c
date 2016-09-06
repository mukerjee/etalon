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
#define TOT_HDR_LEN 28

unsigned int pkt_mangle_begin (void *priv,
                               struct sk_buff *skb,
                               const struct nf_hook_state *state) { 
  struct iphdr *iph;
  struct udphdr *udph;

  unsigned int data_len;
  unsigned char extra_data[] = "12345";
  unsigned int extra_data_len;
  unsigned int tot_data_len;

  unsigned char *ptr;

  __u16 dst_port;

  if (skb) {
      iph = (struct iphdr *) skb_header_pointer(skb, 0, 0, NULL);

      if (iph && iph->protocol && iph->protocol == IPPROTO_UDP) {
          udph = (struct udphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);
          dst_port = ntohs(udph->dest);

          if (dst_port == 5001) {
              data_len = skb->len - TOT_HDR_LEN;
              extra_data_len = strlen(extra_data);

              if (((int) extra_data_len) - ((int) skb_tailroom(skb)) > 0) {
                  if (pskb_expand_head(skb, 0, extra_data_len - skb_tailroom(skb),
                                       GFP_ATOMIC)) {
                      // allocation failed
                      // for now just let the packet go out
                      return NF_ACCEPT;
                  }
                  // allocation success
              }
              
              ptr = skb_put(skb, extra_data_len);
              memcpy(ptr, extra_data, extra_data_len);
              
              tot_data_len = data_len + extra_data_len;

              /* Manipulating necessary header fields */
              iph->tot_len = htons(tot_data_len + TOT_HDR_LEN);
              udph->len = htons(tot_data_len + UDP_HDR_LEN);

              /* Calculation of IP header checksum */
              iph->check = 0;
              ip_send_check(iph);

              /* Calculation of UDP checksum */
              udph->check = 0;
              udph->check = ~csum_tcpudp_magic(iph->saddr, iph->daddr,
                                               tot_data_len + UDP_HDR_LEN,
                                               IPPROTO_UDP, 0);
          }
      }
  }
  return NF_ACCEPT;
}

static struct nf_hook_ops pkt_mangle_ops;

int init_module(void) {
  pkt_mangle_ops.hook = pkt_mangle_begin;
  pkt_mangle_ops.hooknum = NF_IP_LOCAL_OUT;
  pkt_mangle_ops.pf = PF_INET;
  pkt_mangle_ops.priority = NF_IP_PRI_FIRST;

  printk(KERN_ALERT "the mangle started ...\n");
  return nf_register_hook(&pkt_mangle_ops);
}

void cleanup_module(void) {
  nf_unregister_hook(&pkt_mangle_ops);
  printk(KERN_ALERT "the mangle stopped ...\n");
} 

MODULE_LICENSE("GPL");
