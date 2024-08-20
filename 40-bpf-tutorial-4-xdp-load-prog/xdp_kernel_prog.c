#include <linux/types.h>

#include <bpf/bpf_helpers.h>
#include <linux/bpf.h>

SEC("xdp")
int xdp_pass_func(struct xdp_md *ctx) {
  bpf_printk("%s", "Pkt pass...");
  return XDP_PASS;
}

SEC("xdp")
int xdp_drop_func(struct xdp_md *ctx) {
  bpf_printk("%s", "Pkt drop...");
  return XDP_DROP;
}

char _license[] SEC("license") = "GPL";