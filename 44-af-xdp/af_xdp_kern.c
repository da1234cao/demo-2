#if 1
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/types.h>
#include <linux/udp.h>
#else
#include "vmlinux.h"
#endif

#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>
#include <xdp/xdp_helpers.h>

struct {
  __uint(type, BPF_MAP_TYPE_XSKMAP);
  __type(key, __u32);
  __type(value, __u32);
  __uint(max_entries, 64);
} xsks_map SEC(".maps");

SEC("xdp")
int xdp_sock_prog(struct xdp_md *ctx) {
  __u64 data_end = ctx->data_end;
  __u64 data = ctx->data;
  struct ethhdr *eth = (struct ethhdr *)data;
  struct iphdr *ip;
  __u16 h_proto;

  // Check Ethernet header
  if ((__u64)eth + sizeof(*eth) > data_end)
    return XDP_PASS;

  h_proto = eth->h_proto;
  // Only handle IPv4
  if (h_proto != bpf_htons(ETH_P_IP))
    return XDP_PASS;

  ip = (struct iphdr *)((__u64)eth + sizeof(*eth));
  if ((__u64)ip + sizeof(*ip) > data_end)
    return XDP_PASS;

  // Only handle ICMP protocol
  if (ip->protocol != IPPROTO_ICMP)
    return XDP_PASS;

  // Redirect to XSK if conditions are met
  int index = ctx->rx_queue_index;
  return bpf_redirect_map(&xsks_map, index, XDP_PASS);
}

char LICENSE[] SEC("license") = "GPL";
