#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/net.h>
#include <linux/in.h>
#include <net/ip.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("2021320097");
MODULE_DESCRIPTION("2021320097 Netfilter Packet Redirect Module");

#define TARGET_PORT 8080    
#define REDIRECT_IP 0x0A000102 
#define REDIRECT_PORT 8083      

static unsigned int my_nf_hookfn(void *priv,
                              struct sk_buff *skb,
                              const struct nf_hook_state *state) {
    struct iphdr *iph;
    struct udphdr *udph;

    if (!skb)
        return NF_ACCEPT;

    iph = ip_hdr(skb);
    if (iph->protocol != IPPROTO_UDP)
        return NF_ACCEPT;

    udph = udp_hdr(skb);

    if (ntohs(udph->dest) == TARGET_PORT) {
  
        printk(KERN_INFO "FORWARDING: UDP:%pI4:%u;%pI4:%u\n",
            &iph->saddr, ntohs(udph->source),
            &iph->daddr, ntohs(udph->dest));

  
        iph->daddr = htonl(REDIRECT_IP);
        udph->dest = htons(REDIRECT_PORT);

        udph->check = 0;
        skb->csum = 0;
        skb->ip_summed = CHECKSUM_NONE;
        ip_send_check(iph);

        
        printk(KERN_INFO "MODIFIED:   UDP:%pI4:%u;%pI4:%u\n",
            &iph->saddr, ntohs(udph->source),
            &iph->daddr, ntohs(udph->dest));
    }

    return NF_ACCEPT;
}

static struct nf_hook_ops nfho = {
    .hook     = my_nf_hookfn,
    .hooknum  = NF_INET_PRE_ROUTING,
    .pf       = PF_INET,
    .priority = NF_IP_PRI_FIRST
};

static int __init my_init(void) {
    return nf_register_net_hook(&init_net, &nfho);
}

static void __exit my_exit(void) {
    nf_unregister_net_hook(&init_net, &nfho);
}

module_init(my_init);
module_exit(my_exit);