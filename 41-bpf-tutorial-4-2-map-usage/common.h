#pragma once

#include <linux/bpf.h>
#include <linux/types.h>

struct data_record {
  __u64 rx_packets;
  __u64 rx_bytes;
};

#ifndef XDP_ACTION_MAX
#define XDP_ACTION_MAX (XDP_REDIRECT + 1)
#endif