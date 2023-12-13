#include "common.h"
#include <rte_eal.h>
#include <rte_errno.h>

int main(int argc, char *argv[]) {
  int ret;

  // 创建pipe管道
  int fd[2];
  ret = pipe(fd);
  if (ret < 0) {
    fprintf(stderr, "Failed to execute pipe function\n");
    return -1;
  }

  ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    printf("Error message: %s\n", rte_strerror(rte_errno));
    return -1;
  }

  // 获取当前程序的名称
  char hello_msg_send[256];
  get_process_name(hello_msg_send, sizeof(hello_msg_send));
  strncat(hello_msg_send, ":Hi, I am secondary.",
          sizeof(hello_msg_send) - strlen(hello_msg_send) - 1);

  // 构建发送消息
  struct rte_mp_msg hello;
  strncpy(hello.name, "hello_register", sizeof(hello.name) - 1);
  strncpy(hello.param, hello_msg_send, sizeof(hello.param) - 1);
  hello.len_param = strlen(hello_msg_send);
  hello.fds[0] = fd[0];
  hello.num_fds = 1;

  ret = rte_mp_sendmsg(&hello);
  if (ret < 0) {
    printf("Error message: %s\n", rte_strerror(rte_errno));
    return -1;
  }

  // 通过pipe的fd,给primary发送内容
  const char *close_msg = "I(secondary) will close";
  int send_len = strlen(close_msg);
  int n = 0;
  while (n < send_len) {
    n += write(fd[1], close_msg + n, sizeof(close_msg));
  }

  close(fd[1]);

  return 0;
}