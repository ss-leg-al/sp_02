#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>


char LICENSE[] SEC("license") = "GPL";
#define ETH_P_IP 0x0800
#define TARGET_PORT 8080
#define REDIRECT_PORT 8083
#define REDIRECT_IP 0x0a000102 


static __always_inline __u16 csum_fold_helper(__u32 csum) {
    #pragma unroll
    for (int i = 0; i < 4; i++) {
        if ((csum >> 16) == 0)
            break;
        csum = (csum & 0xFFFF) + (csum >> 16);
    }
    return ~csum;
}

static __always_inline __u32 sum16(const void *start, __u32 size, const void *data_end) {
    __u32 sum = 0;
    const __u16 *ptr = start;

    #pragma unroll
    for (int i = 0; i < 256; i++) {
        if ((void *)(ptr + 1) > data_end || size < 2)
            break;
        sum += *ptr++;
        size -= 2;
    }

    if (size == 1) {
        if ((void *)ptr >= data_end) return 0;
        sum += *(__u8 *)ptr;
    }

    return sum;
}

SEC("xdp")
int redirect_udp(struct xdp_md *ctx) {
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;

    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end) return XDP_PASS;
    if (bpf_ntohs(eth->h_proto) != ETH_P_IP) return XDP_PASS;

    struct iphdr *iph = (void *)(eth + 1);
    if ((void *)(iph + 1) > data_end) return XDP_PASS;
    if (iph->protocol != IPPROTO_UDP) return XDP_PASS;

    struct udphdr *udph = (void *)iph + iph->ihl * 4;
    if ((void *)(udph + 1) > data_end) return XDP_PASS;
    if (bpf_ntohs(udph->dest) != TARGET_PORT) return XDP_PASS;

    //bpf_printk("Before: %pI4:%d -> %pI4:%d\n", 
      //         &iph->saddr, bpf_ntohs(udph->source), 
        //       &iph->daddr, bpf_ntohs(udph->dest));

    iph->daddr = bpf_htonl(REDIRECT_IP);
    udph->dest = bpf_htons(REDIRECT_PORT);

    iph->check = 0;
    __u32 ip_csum = sum16(iph, iph->ihl * 4, data_end);
    iph->check = csum_fold_helper(ip_csum);

    __u16 udp_len = bpf_ntohs(udph->len);
    udph->check = 0;

    __u32 pseudo = 0;
    pseudo += (iph->saddr >> 16) + (iph->saddr & 0xffff);
    pseudo += (iph->daddr >> 16) + (iph->daddr & 0xffff);
    pseudo += bpf_htons(IPPROTO_UDP);
    pseudo += udph->len;

    __u32 udp_csum = pseudo + sum16(udph, udp_len, data_end);
    udph->check = csum_fold_helper(udp_csum);

    //bpf_printk("After: Redirected to %pI4:%d\n", &iph->daddr, REDIRECT_PORT);

    return XDP_PASS;
}