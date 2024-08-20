#include <argp.h>
#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <bpf/libbpf.h>
#include <xdp/libxdp.h>

#define PROG_NAME_MAXSIZE 32

static char ifname[IF_NAMESIZE] = {};
static const char *filename = "xdp_kernel_prog.o";
static char prog_name[PROG_NAME_MAXSIZE] = {};
static volatile bool exiting = false;

static int parse_opt(int key, char *arg, struct argp_state *state) {
  switch (key) {
  case 'd':
    snprintf(ifname, sizeof(ifname), "%s", arg);
    break;
  case 'p':
    snprintf(prog_name, sizeof(prog_name), "%s", arg);
    break;
  }
  return 0;
}

static void sig_handler(int sig) { exiting = true; }

int main(int argc, char *argv[]) {
  int ret = 0;
  struct bpf_object *obj;

  struct argp_option options[] = {
      {"dev", 'd', "DEV NAME", 0, "set the network card name"},
      {"prog", 'p', "program name", 0, "set the program"},
      {0},
  };

  struct argp argp = {
      .options = options,
      .parser = parse_opt,
  };

  argp_parse(&argp, argc, argv, 0, 0, 0);

  // check parameter
  printf("choice dev: %s\n", ifname);
  int ifindex = if_nametoindex(ifname);
  if (ifindex == 0) {
    perror("if_nametoindex failed");
    exit(EXIT_FAILURE);
  }
  printf("%s's index %d\n", ifname, ifindex);

  /* Cleaner handling of Ctrl-C */
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  obj = bpf_object__open(filename);
  if (obj == NULL) {
    perror("bpf_object__open failed");
    exit(EXIT_FAILURE);
  }

  struct xdp_program_opts prog_opts = {};
  prog_opts.sz = sizeof(struct xdp_program_opts);
  prog_opts.obj = obj;
  prog_opts.prog_name = prog_name;

  struct xdp_program *prog = xdp_program__create(&prog_opts);
  if (prog == NULL) {
    perror("xdp_program__create failed.");
    exit(EXIT_FAILURE);
  }

  ret = xdp_program__attach(prog, ifindex, XDP_MODE_UNSPEC, 0);
  if (ret != 0) {
    perror("xdp_program__attach failed.");
    exit(EXIT_FAILURE);
  }

  while (!exiting) {
    sleep(1);
  }

cleanup:
  struct xdp_multiprog *mp = xdp_multiprog__get_from_ifindex(ifindex);
  ret = xdp_multiprog__detach(mp);
  if (ret != 0) {
    perror("xdp_multiprog__detach failed.");
    exit(EXIT_FAILURE);
  }

  bpf_object__close(obj);
}