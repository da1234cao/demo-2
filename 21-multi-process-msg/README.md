# DPDK多进程之间的通信

## 前言

DPDK的主进程和辅助进程之间共享大页内存。关于DPDK多进程的支持文档介绍见：[47. 多进程支持](https://doc.dpdk.org/guides/prog_guide/multi_proc_support.html)。

本文介绍本机DPDK的主进程和辅助进程之间交换短消息的API的使用。

---

## 本机DPDK IPC API介绍

具体的API接口使用，见[API 手册](https://doc.dpdk.org/api/rte__eal_8h.html#a2d6d7b2f93fc0ca8c91d7f57daacced8)。

第一种是消息。相关函数如下：
* `int rte_mp_sendmsg (struct rte_mp_msg *msg)`：发送消息。辅助进程调用该函数，发送单播消息到主进程。主进程调用该函数，发送广播消息到所有辅助进程。

第二种是请求。相关函数如下：
* `int 	rte_mp_request_sync (struct rte_mp_msg *req, struct rte_mp_reply *reply, const struct timespec *ts)`: 同步请求，阻塞直到收返回内容，可以设置等待时长。
* `int 	rte_mp_request_async (struct rte_mp_msg *req, const struct timespec *ts, rte_mp_async_reply_t clb)`: 异步请求。不会阻塞。
* `int 	rte_mp_action_register (const char *name, rte_mp_t action)`: 注册消息/请求到来时，对应的响应函数。
* `void 	rte_mp_action_unregister (const char *name)`: 取消注册的响应函数。
* `int 	rte_mp_reply (struct rte_mp_msg *msg, const char *peer)`: 响应函数中，调用该函数，对请求做出回复。

不管时消息还是请求，都需要填充`rte_mp_msg`这个结构体。要填充的字段列表如下：
* name- 消息名称。该名称必须与接收者的回调名称匹配。
* param- 消息数据（最多 256 字节）。
* len_param- 消息数据的长度。
* fds- 与数据一起传递的文件描述符（最多 8 个 fd）。
* num_fds- 要发送的文件描述符的数量。

可以看到，主辅进程之间可以传递文件描述符，这就很厉害了。可以看到消息一次最多传递256个字符，传递数据能力有限。但是可以通过传递文件描述符(如，[socketpair](https://man7.org/linux/man-pages/man2/socketpair.2.html)创建的描述符)，通信起来就很方便了。

## demo演示

不太会用，大概写了一个demo：(1)辅助进程发送消息给主进程，其中包含一段字符串和一个文件描述符。(2)主进程收到消息后，阻塞读取文件描述符。(3)辅助进程通过fd给主进程发送消息，最后关闭fd(发送EOF)。

完整代码见仓库。

辅助进程代码如下。

```c
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
```

主进程代码如下。

```c
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
```

运行输出如下。

```shell
sudo ./primary 

sudo ./secondary --proc-type=secondary

# 主进程输入如下
recv:secondary:Hi, I am secondary.
recv len 29
I(secondary) will close
```