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

unsigned int pkt_add_begin (void *priv,
                            struct sk_buff *skb,
                            const struct nf_hook_state *state) { 
  struct iphdr *iph;
  struct tcphdr *tcph;
  struct udphdr *udph;

  unsigned short data_len;
  int data_len_s;
  unsigned int tcp_hdr_len;
  int extra_space_needed;

  unsigned char *ptr;

  __u16 dst_port;

  struct sk_buff *new_skb;

  if (skb) {
      iph = (struct iphdr *) skb_header_pointer(skb, 0, 0, NULL);

      if (iph && iph->protocol == IPPROTO_TCP) {
          tcph = (struct tcphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);
          dst_port = ntohs(tcph->dest);

          if (dst_port == APP_PORT && iph->tos == 1) {
              //new_skb = skb_copy(skb, GFP_KERNEL);
              new_skb = skb;
              tcp_hdr_len = tcph->doff * 4;

              data_len_s = (int)new_skb->len - (int)IP_HDR_LEN
                  - (int)tcp_hdr_len - (int)APP_HDR_LEN;
              if (data_len_s < (int)sizeof(data_len)) { // e.g., SYN, App control
                  if (DEBUG)
                      printk(KERN_INFO "short packet: %d\n", new_skb->len);
                  return NF_ACCEPT;
              }

              if (DEBUG)
                  printk(KERN_INFO "long packet\n");

              // what data len should be
              data_len = ntohs(*(unsigned short *)(skb_transport_header(new_skb)
                                                   + tcp_hdr_len + APP_HDR_LEN));
              //data_len = 8000;
              if (DEBUG)
                  printk(KERN_INFO "data_len = %d\n", data_len);

              extra_space_needed = (int)data_len - (int)sizeof(data_len)
                  - (int)skb_tailroom(new_skb);

              if (extra_space_needed  > 0) {
                  if (pskb_expand_head(new_skb, 0, extra_space_needed, GFP_ATOMIC)) {
                      // allocation failed
                      // for now just let the packet go out
                      if (DEBUG)
                          printk(KERN_INFO "couldn't allocate data\n");

                      return NF_ACCEPT;
                  }
                  if (DEBUG)
                      printk(KERN_INFO "allocation success tailroom=%d\n",
                             skb_tailroom(new_skb));
                  
                  iph = (struct iphdr *) skb_header_pointer(new_skb, 0, 0, NULL);
                  tcph = (struct tcphdr *) skb_header_pointer(new_skb, IP_HDR_LEN, 0, NULL);

                  // allocation success
              }

              skb_linearize(new_skb);

              /* if (skb_is_nonlinear(skb)) { */
              /*     if (DEBUG) */
              /*         printk(KERN_INFO "paged packet?\n"); */

              /*     return NF_ACCEPT; // let packet go out for now */
              /* } */
              
              // add the amount of data the packet told us to add, but take into
              // account that the packet already has one unsigned short worth of data
              // in it (i.e., it's data_len). We want a pointer to where data_len
              // is within the packet.
              ptr = skb_put(new_skb, data_len - sizeof(data_len)) - sizeof(data_len);

              if (DEBUG)
                  printk(KERN_INFO "ip tot should be %d\n", data_len + APP_HDR_LEN
                         + tcp_hdr_len + IP_HDR_LEN);

              // Manipulating necessary header fields
              iph->tot_len = htons(data_len + APP_HDR_LEN + tcp_hdr_len + IP_HDR_LEN);

              if (DEBUG)
                  printk(KERN_INFO "ip tot_len is %d\n", ntohs(iph->tot_len));
          }
      } else if (iph && iph->protocol == IPPROTO_UDP) {
          udph = (struct udphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);
          dst_port = ntohs(udph->dest);

          if (DEBUG)
              printk(KERN_INFO "got udp, dst_port = %d\n", dst_port);

          if (dst_port == APP_PORT && iph->tos == 1) {
              data_len_s = (int)skb->len - (int)IP_HDR_LEN
                  - (int)UDP_HDR_LEN - (int)APP_HDR_LEN;
              if (data_len_s < (int)sizeof(data_len)) { // e.g., App control
                  if (DEBUG)
                      printk(KERN_INFO "short packet\n");
                  return NF_ACCEPT;
              }

              // what data len should be
              data_len = ntohs(*(unsigned short *)(skb_transport_header(skb)
                                                   + UDP_HDR_LEN + APP_HDR_LEN));
              if (DEBUG)
                  printk(KERN_INFO "data_len = %d\n", data_len);

              extra_space_needed = (int)data_len - (int)sizeof(data_len)
                  - (int)skb_tailroom(skb);

              if (extra_space_needed  > 0) {
                  if (pskb_expand_head(skb, 0, extra_space_needed, GFP_ATOMIC)) {
                      // allocation failed
                      // for now just let the packet go out
                      if (DEBUG)
                          printk(KERN_INFO "couldn't allocate data\n");

                      return NF_ACCEPT;
                  }
                  if (DEBUG)
                      printk(KERN_INFO "allocation success tailroom=%d\n",
                             skb_tailroom(skb));
                  
                  iph = (struct iphdr *) skb_header_pointer(skb, 0, 0, NULL);
                  udph = (struct udphdr *) skb_header_pointer(skb, IP_HDR_LEN, 0, NULL);

                  // allocation success
              }

              skb_linearize(skb);
              /* if (skb_is_nonlinear(skb)) { */
              /*     if (DEBUG) */
              /*         printk(KERN_INFO "paged packet?\n"); */

              /*     return NF_ACCEPT; // let packet go out for now */
              /* } */

              // add the amount of data the packet told us to add, but take into
              // account that the packet already has one unsigned short worth of data
              // in it (i.e., it's data_len). We want a pointer to where data_len
              // is within the packet.
              ptr = skb_put(skb, data_len - sizeof(data_len)) - sizeof(data_len);

              if (DEBUG)
                  printk(KERN_INFO "ip tot should be %d\n", data_len + APP_HDR_LEN
                         + UDP_HDR_LEN + IP_HDR_LEN);

              // Manipulating necessary header fields
              iph->tot_len = htons(data_len + APP_HDR_LEN + UDP_HDR_LEN + IP_HDR_LEN);
              udph->len = htons(data_len + APP_HDR_LEN + UDP_HDR_LEN);

              if (DEBUG) {
                  printk(KERN_INFO "ip tot_len is %d\n", ntohs(iph->tot_len));
                  printk(KERN_INFO "udp len is %d\n", ntohs(udph->len));
              }
          }
      }
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
