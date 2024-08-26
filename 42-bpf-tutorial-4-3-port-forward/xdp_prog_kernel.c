#include <linux/types.h>

#include <bpf/bpf_helpers.h>
#include <linux/bpf.h>
#include <xdp/parsing_helpers.h>

#include "common.h"

// Port forwarding mapping table
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __type(key, unsigned short);
  __type(value, struct data_record);
  __uint(max_entries, 100);
} xdp_stats_map SEC(".maps");

// Record the number of packets from different ports
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __type(key, unsigned short);
  __type(value, unsigned short);
  __uint(max_entries, 100);
} xdp_forward_map SEC(".maps");

static __always_inline __u16 csum_incremental_compute(__u16 old_value,
                                                      __u16 new_value,
                                                      __u16 old_csum) {
  __u32 csum = 0;
  csum = ~old_csum + ~old_value + new_value;

  csum = (csum & 0xffff) + (csum >> 16);
  return ~((csum & 0xffff) + (csum >> 16));

  // 上面计算出来的check sum 总是比正确的check sum 大1;
  // 结果减一,即可得到正确的check sum.
  // return ~((csum & 0xffff) + (csum >> 16)) - 1;
}

SEC("xdp")
int xdp_port_forward(struct xdp_md *ctx) {
  void *data_end = (void *)(long)ctx->data_end;
  void *data = (void *)(long)ctx->data;

  struct hdr_cursor nh;
  struct ethhdr *eth;
  struct iphdr *iphdr;
  struct ipv6hdr *ipv6hdr;
  struct udphdr *udphdr;
  struct tcphdr *tcphdr;

  int eth_type;
  int ip_type;

  nh.pos = data;

  /* Parse Ethernet and IP/IPv6 headers */
  eth_type = parse_ethhdr(&nh, data_end, &eth);
  if (eth_type == bpf_htons(ETH_P_IP)) {
    ip_type = parse_iphdr(&nh, data_end, &iphdr);
  } else if (eth_type == bpf_htons(ETH_P_IPV6)) {
    ip_type = parse_ip6hdr(&nh, data_end, &ipv6hdr);
  } else {
    bpf_printk("Current ip type, not processed:%d", ip_type);
    goto out;
  }
  bpf_printk("Current ip type:%d", ip_type);

  if (ip_type == IPPROTO_UDP) {
    bpf_printk("No udp packets are currently being processed");
    goto out;
    // TODO !
  } else if (ip_type == IPPROTO_TCP) {
    if (parse_tcphdr(&nh, data_end, &tcphdr) < 0) {
      bpf_printk("parse_tcphdr failed: insufficient data.");
      goto out;
    }
    bpf_printk("parse_tcphdr success.");

    // Look up the mapping table to determine which port the current data packet
    // should be forwarded to
    unsigned short key = bpf_ntohs(tcphdr->dest);
    unsigned short *forward_port =
        (unsigned short *)bpf_map_lookup_elem(&xdp_forward_map, &key);
    if (forward_port == NULL) {
      bpf_printk("No rules for destination port %d", key);
      goto out;
    }

    bpf_printk("port %d forward to port %d", key, *forward_port);

    // Modify the data packet && recalculate the check sum

    // method 1: self compute incremental csum. failed.
    bpf_printk("old check sum  %d", tcphdr->check);
    // unsigned short old_dest = tcphdr->dest;
    // unsigned short new_dest = bpf_htons(*forward_port);
    // tcphdr->dest = new_dest;
    // tcphdr->check = csum_incremental_compute(old_dest, new_dest,
    // tcphdr->check);

    // method 2: call bpf_csum_diff() compute csum. failed.
    struct tcphdr tcphdr_old;
    tcphdr_old = *tcphdr;
    tcphdr->dest = bpf_htons(*forward_port);
    __u32 csum = bpf_csum_diff((__be32 *)&tcphdr_old, 4, (__be32 *)tcphdr, 4,
                               ~tcphdr->check);
    csum = (csum & 0xffff) + (csum >> 16);
    csum = ((csum & 0xffff) + (csum >> 16));
    // tcphdr->check = ~csum;

    bpf_printk("new check sum  %d", tcphdr->check);

    // Recording Statistics
    __u32 bytes = (char *)data_end - (char *)data;
    struct data_record *record = bpf_map_lookup_elem(&xdp_stats_map, &key);
    if (record != NULL) {
      record->rx_packets++;
      record->rx_bytes += bytes;
    } else {
      struct data_record record_tmp = {};
      record_tmp.rx_packets++;
      record_tmp.rx_bytes += bytes;
      if (bpf_map_update_elem(&xdp_stats_map, &key, &record_tmp, 0) < 0) {
        goto out;
      }
    }
  }

out:
  return XDP_PASS;
}

char _license[] SEC("license") = "GPL";