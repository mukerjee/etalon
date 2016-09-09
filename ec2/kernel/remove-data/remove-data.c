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
#define APP_HDR_LEN 36
#define APP_PORT 5001

#define DEBUG 0

unsigned int pkt_remove_begin (void *priv,
                               struct sk_buff *skb,
                               const struct nf_hook_state *state) { 
  struct iphdr *iph;
  struct tcphdr *tcph;
  struct udphdr *udph;

  unsigned short data_len;
  int data_len_s;
  unsigned int tcp_hdr_len;

  __u16 dst_port;

  if (skb) {
      iph = (struct iphdr *) skb_header_pointer(skb, 0, 0, NULL);

      if (iph && iph->protocol == IPPROTO_TCP) {
          tcph = (struct tcphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);
          dst_port = ntohs(tcph->dest);

          if (dst_port == APP_PORT) {
              tcp_hdr_len = tcph->doff * 4;
              data_len_s = (int)skb->len - (int)IP_HDR_LEN
                  - (int)tcp_hdr_len - (int)APP_HDR_LEN;

              if (data_len_s < (int)sizeof(data_len)) {
                  if (DEBUG)
                      printk(KERN_INFO "short packet: %d\n", skb->len);
                  return NF_ACCEPT; // e.g., SYN, APP Control
              }

              data_len = (unsigned short) data_len_s;


              skb_linearize(skb);
              
              if (DEBUG)
                  printk(KERN_INFO "data_len: %d, new len: %ld\n", data_len,
                         IP_HDR_LEN + tcp_hdr_len + APP_HDR_LEN + sizeof(data_len));

              // put the original data len into first two bytes of app payload
              *(unsigned short *)(skb_transport_header(skb) + tcp_hdr_len +
                                  APP_HDR_LEN) = htons(data_len);
              
              skb->len = IP_HDR_LEN + tcp_hdr_len + APP_HDR_LEN + sizeof(data_len);

              iph = (struct iphdr *) skb_header_pointer(skb, 0, 0, NULL);
              tcph = (struct tcphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);

              iph->tos = 1; // mark as removed

              // Manipulating necessary header fields
              iph->tot_len = htons(IP_HDR_LEN + tcp_hdr_len
                                   + APP_HDR_LEN + sizeof(data_len));

              // Calculation of IP header checksum
              iph->check = 0;
              ip_send_check(iph);

              // Calculation of UDP checksum
              tcph->check = 0;
              data_len = skb->len - IP_HDR_LEN - tcp_hdr_len - APP_HDR_LEN;
              tcph->check = ~csum_tcpudp_magic(iph->saddr, iph->daddr,
                                               tcp_hdr_len + APP_HDR_LEN
                                               + data_len,
                                               IPPROTO_TCP, 0);
          }
      } else if (iph && iph->protocol == IPPROTO_UDP) {
          udph = (struct udphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);
          dst_port = ntohs(udph->dest);

          if (dst_port == APP_PORT) {
              data_len_s = (int)skb->len - (int)IP_HDR_LEN
                  - (int)UDP_HDR_LEN - (int)APP_HDR_LEN;

              if (data_len_s < (int)sizeof(data_len)) {
                  if (DEBUG)
                      printk(KERN_INFO "short packet\n");
                  return NF_ACCEPT; // e.g., APP Control
              }

              data_len = (unsigned short)data_len_s;

              skb_linearize(skb);
              /* if (skb_is_nonlinear(skb)) { */
              /*     if (DEBUG) */
              /*         printk(KERN_INFO "paged udp skb: %d\n", skb->data_len); */

              /*     return NF_ACCEPT; */
              /* } */


              if (DEBUG)
                  printk(KERN_INFO "data_len: %d\n", data_len);

              // put the original data len into first two bytes of packet payload
              *(unsigned short *)(skb_transport_header(skb) + UDP_HDR_LEN +
                                  APP_HDR_LEN) = htons(data_len);

              skb->len = IP_HDR_LEN + UDP_HDR_LEN + APP_HDR_LEN + sizeof(data_len);

              iph->tos = 1; // mark as removed

              // Manipulating necessary header fields
              iph->tot_len = htons(IP_HDR_LEN + UDP_HDR_LEN
                                   + APP_HDR_LEN + sizeof(data_len));
              udph->len = htons(UDP_HDR_LEN + APP_HDR_LEN + sizeof(data_len));

              // Calculation of IP header checksum
              iph->check = 0;
              ip_send_check(iph);

              // Calculation of UDP checksum
              udph->check = 0;
              udph->check = ~csum_tcpudp_magic(iph->saddr, iph->daddr,
                                               UDP_HDR_LEN + APP_HDR_LEN
                                               + sizeof(data_len),
                                               IPPROTO_UDP, 0);
          }
      }
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
