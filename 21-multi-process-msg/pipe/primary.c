#include "common.h"
#include <rte_eal.h>
#include <rte_errno.h>

int secondary_fd = -1;

int hello_msg_reply(const struct rte_mp_msg *msg, const void *peer) {
  printf("recv:");
  printf("%s\n", (char *)msg->param);
  printf("recv len %d\n", msg->len_param);
  secondary_fd = msg->fds[0];
}

int main(int argc, char *argv) {
  int ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    printf("Error message: %s\n", rte_strerror(rte_errno));
    return -1;
  }

  rte_mp_action_register("hello_register", hello_msg_reply);

  while (secondary_fd < 0) {
    sleep(1);
  }

  char buf[256];
  while (read(secondary_fd, buf, sizeof(buf) - 1)) {
    printf("%s\n", buf);
  }
  return 0;
}