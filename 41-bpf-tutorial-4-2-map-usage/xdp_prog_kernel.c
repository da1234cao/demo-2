#include <linux/types.h>

#include <bpf/bpf_helpers.h>

#include "common.h"

struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __type(key, __u32);
  __type(value, struct data_record);
  __uint(max_entries, XDP_ACTION_MAX);
} xdp_stats_map SEC(".maps");

SEC("xdp")
int xdp_stats_func(struct xdp_md *ctx) {
  struct data_record *record;
  __u32 key = XDP_PASS;

  record = bpf_map_lookup_elem(&xdp_stats_map, &key);
  if (record == NULL) {
    return XDP_ABORTED;
  }

  __u32 bytes = ctx->data_end - ctx->data;

  record->rx_packets++;
  record->rx_bytes += bytes;

  return XDP_PASS;
}

char _license[] SEC("license") = "GPL";