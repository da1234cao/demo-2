[toc]

# ebpf教程(4.2):bpf map的使用 -- 统计网速

## 前言

前置阅读：
- [ebpf教程(4.1):XDP程序的加载-CSDN博客](https://blog.csdn.net/sinat_38816924/article/details/141365581)

> map 可用于内核 BPF 程序和用户应用程序之间实现双向的数据交换， 是 BPF 技术中的重要基础数据结构。

本文使用 `BPF_MAP_TYPE_PERCPU_ARRAY` 这一类型bpf map，将指定网卡的网速信息上传到用户层。用户层不断的从map中提取网卡的网速信息，并打印。

---

## 统计网卡的速度

代码修改自：
- [xdp-tutorial/basic03-map-counter at master · xdp-project/xdp-tutorial](https://github.com/xdp-project/xdp-tutorial/tree/master/basic03-map-counter)
- [xdp-tutorial/basic02-prog-by-name/README.org at master · xdp-project/xdp-tutorial](https://github.com/xdp-project/xdp-tutorial/tree/master/basic04-pinning-maps)

---

### BFP 程序

BPF 程序，将接收到的数据包个数和数据包长度写入map。

```c
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
```

---

### 用户层程序

用户层程序：加载 BPF程序，并打印网速。

```c
#include <argp.h>
#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <bpf/libbpf.h>
#include <xdp/libxdp.h>

#include "common.h"

#define PROG_NAME_MAXSIZE 32
#define NANOSEC_PER_SEC 1000000000 /* 10^9 */

struct main_config {
  char filename[PATH_MAX];
  char prog_name[PROG_NAME_MAXSIZE];
  char ifname[IF_NAMESIZE];
  int ifindex;
  int interval;
};

struct data_record_user {
  struct data_record record;
  struct timespec ts;
};

static struct main_config main_config;
static volatile bool exiting = false;
struct bpf_object *obj = NULL;

static int parse_opt(int key, char *arg, struct argp_state *state) {
  switch (key) {
  case 'd':
    snprintf(main_config.ifname, sizeof(main_config.ifname), "%s", arg);
    break;
  case 0:
    main_config.interval = atoi(arg);
    break;
  }
  return 0;
}

static void sig_handler(int sig) { exiting = true; }

int load_bpf_and_xdp_attach() {
  int ret = 0;

  obj = bpf_object__open(main_config.filename);
  if (obj == NULL) {
    perror("bpf_object__open failed");
    exit(EXIT_FAILURE);
  }

  struct xdp_program_opts prog_opts = {};
  prog_opts.sz = sizeof(struct xdp_program_opts);
  prog_opts.obj = obj;
  prog_opts.prog_name = main_config.prog_name;

  struct xdp_program *prog = xdp_program__create(&prog_opts);
  if (prog == NULL) {
    perror("xdp_program__create failed");
    exit(EXIT_FAILURE);
  }

  ret = xdp_program__attach(prog, main_config.ifindex, XDP_MODE_UNSPEC, 0);
  if (ret != 0) {
    perror("xdp_program__attach failed");
    exit(EXIT_FAILURE);
  }

  int prog_fd = xdp_program__fd(prog);
  if (prog_fd < 0) {
    perror("cant get program fd");
    exit(EXIT_FAILURE);
  }

  return prog_fd;
}

static void stats_print(const struct data_record_user *pre_record,
                        const struct data_record_user *record) {
  __u64 packets, bytes;
  double period;
  double pps; /* packets per sec */
  double bps; /* bits per sec */
  int i;

  /* Print for each XDP actions stats */
  char *fmt = "XDP_PASS %'11lld pkts (%'10.0f pps)"
              " %'11lld KB (%'6.0f Mbits/s)\n";

  period =
      (record->ts.tv_sec - pre_record->ts.tv_sec) +
      ((double)(record->ts.tv_nsec - pre_record->ts.tv_nsec) / NANOSEC_PER_SEC);

  // If the clock_gettime function uses CLOCK_REALTIME, this may happen if the
  // system time is modified.
  if (period <= 0)
    return;

  packets = record->record.rx_packets - pre_record->record.rx_packets;
  pps = packets / period;

  bytes = record->record.rx_bytes - pre_record->record.rx_bytes;
  bps = (bytes * 8) / period / 1000000;

  printf(fmt, record->record.rx_packets, pps,
         record->record.rx_bytes * 8 / 1000, bps);
}

void map_get_value_percpu_array(int mapfd, __u32 key,
                                struct data_record_user *value) {
  /* For percpu maps, userspace gets a value per possible CPU */
  int ret;
  int nr_cpus = libbpf_num_possible_cpus();
  struct data_record records[nr_cpus];

  ret = clock_gettime(CLOCK_MONOTONIC, &value->ts);
  if (ret != 0) {
    perror("clock_gettime failed");
    exit(EXIT_FAILURE);
  }

  ret = bpf_map_lookup_elem(mapfd, &key, records);
  if (ret != 0) {
    perror("bpf_map_lookup_elem failed");
    exit(EXIT_FAILURE);
  }

  /* Sum values from each CPU */
  __u64 sum_bytes = 0;
  __u64 sum_pkts = 0;
  for (int i = 0; i < nr_cpus; i++) {
    sum_pkts += records[i].rx_packets;
    sum_bytes += records[i].rx_bytes;
  }

  value->record.rx_packets = sum_pkts;
  value->record.rx_bytes = sum_bytes;
}

void speed_poll(int mapfd) {

  struct data_record_user record = {};
  struct data_record_user pre_record = {};

  sleep(main_config.interval);
  map_get_value_percpu_array(mapfd, XDP_PASS, &record);

  while (!exiting) {
    pre_record = record;
    map_get_value_percpu_array(mapfd, XDP_PASS, &record);
    stats_print(&pre_record, &record);
    sleep(main_config.interval);
  }
}

int main(int argc, char *argv[]) {
  int ret = 0;

  memset(&main_config, 0, sizeof(main_config));
  snprintf(main_config.filename, sizeof(main_config.filename), "%s",
           "xdp_prog_kernel.o");
  snprintf(main_config.prog_name, sizeof(main_config.prog_name), "%s",
           "xdp_stats_func");
  main_config.interval = 2;

  struct argp_option options[] = {
      {"dev", 'd', "device name", 0, "Set the network card name"},
      {"interval", 0, "statistical interval", 0,
       "Set the statistical interval"},
      {0},
  };

  struct argp argp = {
      .options = options,
      .parser = parse_opt,
  };

  argp_parse(&argp, argc, argv, 0, 0, 0);

  // check parameter
  int ifindex = if_nametoindex(main_config.ifname);
  if (ifindex == 0) {
    perror("if_nametoindex failed");
    exit(EXIT_FAILURE);
  }
  main_config.ifindex = ifindex;

  // print config
  printf("prog name: %s\n", main_config.prog_name);
  printf("choice dev: %s\n", main_config.ifname);
  printf("%s's index: %d\n", main_config.ifname, ifindex);
  printf("sampling interval for statistics: %d\n", main_config.interval);

  /* Cleaner handling of Ctrl-C */
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  int prog_fd = load_bpf_and_xdp_attach();

  // prog->bpf_obj is null;xdp_program__from_fd does not fill in this structure
  // internally
  //   struct xdp_program *prog = xdp_program__from_fd(prog_fd);
  //   struct bpf_object *obj = xdp_program__bpf_obj(prog);

  if (obj == NULL) {
    perror("xdp_program__bpf_obj failed");
    exit(EXIT_FAILURE);
  }

  struct bpf_map *map = bpf_object__find_map_by_name(obj, "xdp_stats_map");
  if (map == NULL) {
    perror("bpf_object__find_map_by_name failed");
    exit(EXIT_FAILURE);
  }

  int map_fd = bpf_map__fd(map);
  speed_poll(map_fd);

cleanup:
  struct xdp_multiprog *mp = xdp_multiprog__get_from_ifindex(ifindex);
  ret = xdp_multiprog__detach(mp);
  if (ret != 0) {
    perror("xdp_multiprog__detach failed.");
    exit(EXIT_FAILURE);
  }

  bpf_object__close(obj);
}
```

---

### 构建程序

```cmake
cmake_minimum_required(VERSION 3.10)

project(xdp-load-prog)

find_package(PkgConfig)
pkg_check_modules(LIBBPF REQUIRED libbpf)
pkg_check_modules(LIBXDP REQUIRED libxdp)

find_path(ASM_TYPES_H_PATH NAMES asm/types.h PATHS /usr/include/x86_64-linux-gnu)
if(ASM_TYPES_H_PATH)
    message(STATUS "Found asm/types.h at ${ASM_TYPES_H_PATH}")
    include_directories(${ASM_TYPES_H_PATH})
else()
    message(FATAL_ERROR "asm/types.h not found")
endif()

set(BPF_C_FILE ${CMAKE_CURRENT_SOURCE_DIR}/xdp_prog_kernel.c)
set(BPF_O_FILE ${CMAKE_CURRENT_BINARY_DIR}/xdp_prog_kernel.o)
add_custom_command(OUTPUT ${BPF_O_FILE}
    COMMAND clang -g -O2 -target bpf -I${ASM_TYPES_H_PATH} ${CLANG_SYSTEM_INCLUDES} -c ${BPF_C_FILE} -o ${BPF_O_FILE}
    COMMAND_EXPAND_LISTS
    VERBATIM
    DEPENDS ${BPF_C_FILE}
    COMMENT "[clang] Building BPF file: ${BPF_C_FILE}")

add_custom_target(generate_bpf_obj ALL
    DEPENDS ${BPF_O_FILE}
)

add_executable(xdp_load_and_stats xdp_load_and_stats.c)
target_link_libraries(xdp_load_and_stats PRIVATE ${LIBBPF_LIBRARIES} ${LIBXDP_LIBRARIES})
```

---

### 运行

我使用 `iperf3` 向 `ens19` 网卡发送数据包，以提供流量。

```shell
# 每个两秒，打印下指定网卡的网速
./xdp_load_and_stats --dev=ens19
prog name: xdp_stats_func
choice dev: ens19
ens19's index: 3
sampling interval for statistics: 2
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
libbpf: elf: skipping unrecognized data section(7) xdp_metadata
XDP_PASS           0 pkts (         0 pps)           0 KB (     0 Mbits/s)
XDP_PASS     1142497 pkts (    571216 pps)    13837763 KB (  6918 Mbits/s)
XDP_PASS     2625795 pkts (    741594 pps)    31803445 KB (  8982 Mbits/s)
XDP_PASS     3543266 pkts (    458703 pps)    42915796 KB (  5556 Mbits/s)
```

---

## 更多阅读
- [揭秘 BPF map 前生今世 | Head First eBPF](https://www.ebpf.top/post/map_internal/)
- [BPF 进阶笔记（二）：BPF Map 类型详解：使用场景、程序示例](http://arthurchiao.art/blog/bpf-advanced-notes-2-zh/)

