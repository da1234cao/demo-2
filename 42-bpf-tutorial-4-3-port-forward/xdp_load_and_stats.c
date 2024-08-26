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
  unsigned int port;
  struct data_record record;
  struct timespec ts;
};

static struct main_config main_config;
static volatile bool exiting = false;
struct bpf_object *obj = NULL;
int stats_map_fd;
int forward_map_fd;

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

static void stats_print(const struct data_record_user *record) {
  /* Print for each XDP actions stats */
  char *fmt = "Port %d %'11lld pkts  %'11lld Kbit\n";

  printf(fmt, record->port, record->record.rx_packets,
         record->record.rx_bytes * 8 / 1000);
}

int map_get_value(int mapfd, __u32 key, struct data_record_user *value) {
  int ret;
  struct data_record record;

  ret = bpf_map_lookup_elem(mapfd, &key, &record);
  if (ret != 0) {
    perror("bpf_map_lookup_elem failed");
    return -1;
  }

  value->port = key;
  value->record.rx_packets = record.rx_packets;
  value->record.rx_bytes = record.rx_bytes;

  ret = clock_gettime(CLOCK_MONOTONIC, &value->ts);
  if (ret != 0) {
    perror("clock_gettime failed");
    return -1;
  }

  return 0;
}

void speed_poll() {

  while (!exiting) {
    __u32 key = 0;
    void *keyp = &key, *prev_keyp = NULL;
    struct data_record_user record = {};
    int err;

    while (bpf_map_get_next_key(forward_map_fd, prev_keyp, keyp) == 0) {
      if (map_get_value(stats_map_fd, key, &record) == 0) {
        stats_print(&record);
      }
      prev_keyp = keyp;
    }
    sleep(main_config.interval);
  }
}

int main(int argc, char *argv[]) {
  int ret = 0;

  memset(&main_config, 0, sizeof(main_config));
  snprintf(main_config.filename, sizeof(main_config.filename), "%s",
           "xdp_prog_kernel.o");
  snprintf(main_config.prog_name, sizeof(main_config.prog_name), "%s",
           "xdp_port_forward");
  main_config.interval = 1;

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

  // Clear previous prog
  struct xdp_multiprog *mp = xdp_multiprog__get_from_ifindex(ifindex);
  ret = libxdp_get_error(mp);
  if (!ret) {
    ret = xdp_multiprog__detach(mp);
    if (ret != 0) {
      perror("xdp_multiprog__detach failed.");
      exit(EXIT_FAILURE);
    }
  }

  /* Cleaner handling of Ctrl-C */
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  int prog_fd = load_bpf_and_xdp_attach();

  struct bpf_map *stats_map =
      bpf_object__find_map_by_name(obj, "xdp_stats_map");
  if (stats_map == NULL) {
    perror("bpf_object__find_map_by_name look for xdp_stats_map failed");
    exit(EXIT_FAILURE);
  }
  stats_map_fd = bpf_map__fd(stats_map);

  struct bpf_map *forward_map =
      bpf_object__find_map_by_name(obj, "xdp_forward_map");
  if (forward_map == NULL) {
    perror("bpf_object__find_map_by_name look for xdp_forward_map failed");
    exit(EXIT_FAILURE);
  }
  forward_map_fd = bpf_map__fd(forward_map);

  // Insert a port forwarding rule
  unsigned short port = 10000;
  unsigned short forward_port = 22;
  ret = bpf_map_update_elem(forward_map_fd, &port, &forward_port, 0);
  if (ret != 0) {
    printf("fail to insert forward rule");
    goto cleanup;
  }

  speed_poll();

cleanup:
  mp = xdp_multiprog__get_from_ifindex(ifindex);
  ret = xdp_multiprog__detach(mp);
  if (ret != 0) {
    perror("xdp_multiprog__detach failed.");
    exit(EXIT_FAILURE);
  }

  bpf_object__close(obj);
}