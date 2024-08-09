[toc]

## 前言

[Netlink](https://man7.org/linux/man-pages/man7/netlink.7.html)用于在内核进程和用户空间进程之间传输信息。它有两套API，一套API在用户空间使用，一套API在内核空间使用。它旨在成为比 `ioctl` 更灵活的后继者，主要提供与网络相关的内核配置(kernel configuration)和接口监控(monitoring interfaces)。

[libnl](https://www.infradead.org/~tgr/libnl/)是一组基于 `netlink` 协议的API库的集合。它是 `netlink` 更高层次的封装。

本文介绍netlink API的最简单使用，为之后使用libnl打下基础。

---

## netlink hello world

本节参考自：[Linux Netlink 详解](https://github.com/xgfone/snippet/blob/master/snippet/docs/linux/netlink/netlink-note.md) 、[netlink(7) — Linux manual page](https://man7.org/linux/man-pages/man7/netlink.7.html) 、[Introduction to Netlink](https://docs.kernel.org/userspace-api/netlink/intro.html)

### netlink 用户层接口说明

Netlink通信通过套接字进行，需要先打开套接字。`socket` 的 `domain` 为 `AF_NETLINK`。`type`为 `SOCK_DGRAM` 或者 `SOCK_DGRAM`，netlink 协议不区分这两者。`protocol` 是 `Netlink` 可用的协议，目前已经使用了二十多个，而最多允许定义32个。后面的示例中，我们将使用自定义的协议类型。系统已有的协议类型涉及到具体的网络内容，本文不涉及。

```c
// socket (int domain, int type, int protocol)
fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
```

既然是通信，那至少有接收方和发送方。Netlink的接收方/发送方通常是用户进程/内核。socket网络编程中，通信双方使用四元组(源IP:源port-目的IP:目的port)来在网络中进行唯一标识。Netlink只能用于本机通信，所以不需要IP标识。端口方面，Netlink的端口并不是真的占用一个网络端口号，它只是一个唯一标识。内核的端口号总是0，用户进程的端口号可以为进程ID，如果是在多线程中，可以使用线程ID。

所以我们接下来是给套接字绑定端口号。socket网络编程中，我们通常填充一个 `sockaddr_in` 结构，然后调用bind()函数。`Netlink` 也差不多，不过填充的是 `sockaddr_nl` 结构。

```c
struct sockaddr_nl {
	__kernel_sa_family_t	nl_family;	/* AF_NETLINK	*/
	unsigned short	nl_pad;		/* zero		*/
	__u32		nl_pid;		/* port ID	*/ // 这里填充我们的端口号
        __u32		nl_groups;	/* multicast groups mask */ // 不使用广播时，这里设置为0
};
```

之后，用户空间程序调用 socket的 recv/send 等函数，即可与内核互相发送内容。

```c
send(fd, &request, sizeof(request));
n = recv(fd, &buffer, RSP_BUFFER_SIZE);
```

发送的内容一定得遵循某种格式，这样双方才能互相解析。`Netlink` 使用TLV(type, length, value)格式，即一个消息头，消息头后面时负载，消息头中记录整个消息得长度。`Netlink`协议的标头格式如下。

```c
struct nlmsghdr {
	__u32		nlmsg_len;	/* Length of message including header */
	__u16		nlmsg_type;	/* Message content */
	__u16		nlmsg_flags;	/* Additional flags */
	__u32		nlmsg_seq;	/* Sequence number */
	__u32		nlmsg_pid;	/* Sending process port ID */
};
```

### 示例代码

上面的接口说明是比较无聊的。我们实际来跑一个示例。

#### 示例代码

演示代码来自：[Linux Netlink 详解](https://github.com/xgfone/snippet/blob/master/snippet/docs/linux/netlink/netlink-note.md)

演示代码的功能：
- 用户空间的进程，向内核空间，发送 ”Hello kernel“。
- 内核空间的进程，向该用户进程，回复 ”Hello userspace“ 。

首先是用户空间的代码。

```c
#include <linux/netlink.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define NETLINK_TEST 30
#define MAX_PAYLOAD 1024 /* maximum payload size*/
#define MAX_NL_BUFSIZ NLMSG_SPACE(MAX_PAYLOAD)

// int PORTID = getpid();
int PORTID = 1;

int create_nl_socket(uint32_t pid, uint32_t groups) {
  int fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_TEST);
  if (fd == -1) {
    return -1;
  }

  struct sockaddr_nl addr;
  memset(&addr, 0, sizeof(addr));
  addr.nl_family = AF_NETLINK;
  addr.nl_pid = pid;
  addr.nl_groups = groups;

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    close(fd);
    return -1;
  }

  return fd;
}

ssize_t nl_recv(int fd) {
  char nl_tmp_buffer[MAX_NL_BUFSIZ];
  struct nlmsghdr *nlh;
  ssize_t ret;

  // 设置 Netlink 消息缓冲区
  nlh = (struct nlmsghdr *)&nl_tmp_buffer;
  memset(nlh, 0, MAX_NL_BUFSIZ);

  ret = recvfrom(fd, nlh, MAX_NL_BUFSIZ, 0, NULL, NULL);
  if (ret < 0) {
    return ret;
  }

  printf("==== LEN(%d) TYPE(%d) FLAGS(%d) SEQ(%d) PID(%d)\n\n", nlh->nlmsg_len,
         nlh->nlmsg_type, nlh->nlmsg_flags, nlh->nlmsg_seq, nlh->nlmsg_pid);
  printf("Received data: %s\n", NLMSG_DATA(nlh));
  return ret;
}

int nl_sendto(int fd, void *buffer, size_t size, uint32_t pid,
              uint32_t groups) {
  char nl_tmp_buffer[MAX_NL_BUFSIZ];
  struct nlmsghdr *nlh;

  if (NLMSG_SPACE(size) > MAX_NL_BUFSIZ) {
    return -1;
  }

  struct sockaddr_nl addr;
  memset(&addr, 0, sizeof(addr));
  addr.nl_family = AF_NETLINK;
  addr.nl_pid = pid;       /* Send messages to the linux kernel. */
  addr.nl_groups = groups; /* unicast */

  // 设置 Netlink 消息缓冲区
  nlh = (struct nlmsghdr *)&nl_tmp_buffer;
  memset(nlh, 0, MAX_NL_BUFSIZ);
  nlh->nlmsg_len = NLMSG_LENGTH(size);
  nlh->nlmsg_pid = PORTID;
  memcpy(NLMSG_DATA(nlh), buffer, size);

  return sendto(fd, nlh, NLMSG_LENGTH(size), 0, (struct sockaddr *)&addr,
                sizeof(addr));
}

int main(void) {
  char data[] = "Hello kernel";
  int sockfd = create_nl_socket(PORTID, 0);
  if (sockfd == -1) {
    return 1;
  }

  int ret;
  ret = nl_sendto(sockfd, data, sizeof(data), 0, 0);
  if (ret < 0) {
    printf("Fail to send\n");
    return 1;
  }
  printf("Sent %d bytes\n", ret);

  ret = nl_recv(sockfd);
  if (ret < 0) {
    printf("Fail to receive\n");
  }
  printf("Received %d bytes\n", ret);

  // while (1) {
  // nl_recv(sockfd);
  // nl_sendto(sockfd, data, sizeof(data), 0, 0);
  // }

  return 0;
}
```

然后是内核空间的代码。

```c
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <net/sock.h>

#define NETLINK_TEST 30

static struct sock *nl_sk = NULL;

/*
 * Send the data of `data`, whose length is `size`, to the socket whose port is
 * `pid` through the unicast.
 *
 * @param data: the data which will be sent.
 * @param size: the size of `data`.
 * @param pid: the port of the socket to which will be sent.
 * @return: if successfully, return 0; or, return -1.
 */
int test_unicast(void *data, size_t size, __u32 pid) {
  struct sk_buff *skb_out;
  skb_out = nlmsg_new(size, GFP_ATOMIC);
  if (!skb_out) {
    printk(KERN_ERR "Failed to allocate a new sk_buff\n");
    return -1;
  }

  // struct nlmsghdr* nlmsg_put(struct sk_buff *skb, u32 portid, u32 seq, int
  // type, int len, int flags);
  struct nlmsghdr *nlh;
  nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, size, 0);

  memcpy(nlmsg_data(nlh), data, size);

  // 设置 SKB 的控制块（CB）
  // 控制块是 struct sk_buff
  // 结构特有的，用于每个协议层的控制信息（如：IP层、TCP层） 对于 Netlink
  // 来说，其控制信息是如下结构体： struct netlink_skb_parms {
  //      struct scm_credscreds;  // Skb credentials
  //      __u32portid;            // 发送此SKB的Socket的Port号
  //      __u32dst_group;         // 目的多播组，即接收此消息的多播组
  //      __u32flags;
  //      struct sock*sk;
  // };
  // 对于此结构体，一般只需要设置 portid 和 dst_group 字段。
  // 但对于不同的Linux版本，其结构体会所有变化：早期版本 portid 字段名为 pid。
  // NETLINK_CB(skb_out).pid = pid;
  NETLINK_CB(skb_out).portid = pid;
  NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */

  // 单播/多播
  if (nlmsg_unicast(nl_sk, skb_out, pid) < 0) {
    printk(KERN_INFO "Error while sending a msg to userspace\n");
    return -1;
  }

  return 0;
}
EXPORT_SYMBOL(test_unicast);

static void nl_recv_msg(struct sk_buff *skb) {
  struct nlmsghdr *nlh = (struct nlmsghdr *)skb->data;
  char *data = "Hello userspace";
  printk(KERN_INFO "==== LEN(%d) TYPE(%d) FLAGS(%d) SEQ(%d) PORTID(%d)\n",
         nlh->nlmsg_len, nlh->nlmsg_type, nlh->nlmsg_flags, nlh->nlmsg_seq,
         nlh->nlmsg_pid);
  printk("Received %d bytes: %s\n", nlmsg_len(nlh), (char *)nlmsg_data(nlh));
  test_unicast(data, strlen(data), nlh->nlmsg_pid);
}

static int __init test_init(void) {
  printk("Loading the netlink module\n");

  // This is for 3.8 kernels and above.
  struct netlink_kernel_cfg cfg = {
      .input = nl_recv_msg,
  };

  nl_sk = netlink_kernel_create(&init_net, NETLINK_TEST, &cfg);
  if (!nl_sk) {
    printk(KERN_ALERT "Error creating socket.\n");
    return -10;
  }

  return 0;
}

static void __exit test_exit(void) {
  printk(KERN_INFO "Unloading the netlink module\n");
  netlink_kernel_release(nl_sk);
}

module_init(test_init);
module_exit(test_exit);
MODULE_LICENSE("GPL");
```

#### 安装依赖

```shell
# 当前内核版本
uname -a
Linux bogon 4.18.0-553.8.1.el8_10.x86_64 #1 SMP Tue Jul 2 17:10:26 UTC 2024 x86_64 x86_64 x86_64 GNU/Linux

# https://rockylinux.pkgs.org/8/rockylinux-devel-x86_64/kernel-headers-4.18.0-553.el8_10.x86_64.rpm.html
dnf config-manager --set-enable devel
dnf install kernel-headers-$(uname -r) kernel-devel-$(uname -r)

# 如果安装的kernel-devel和当前正在运行的内核不匹配
# https://forums.rockylinux.org/t/installing-kernel-devel-does-not-match-the-running-kernel/12619/4
dnf upgrade kernel
```

#### 代码的构建

我没有使用MakeFile，而是使用cmake来构建内核模块，所有有点麻烦。具体构建方式见仓库。

- 首先我们得知道MakeFile是如何构建内核模块的。可见：[Linux内核模块编写之1: Hello World及Makefile](http://home.ustc.edu.cn/~fward/blog/linux-module-1-hello-world/)
- 但是我想用cmake构建，这样显得比较厉害。这里有可用的示例：[cmake : specify linux kernel module output build directory](https://stackoverflow.com/questions/50877135/cmake-specify-linux-kernel-module-output-build-directory)、[cmake-kernel-module](https://gitlab.com/christophacham/cmake-kernel-module)
- 上面的cmake-kernel-module，还是有的不好使。当有多个源文件时，不够优雅。使用 configure_file 来配置Kbuild是个好主意。可见：[Using CMake for a Linux kernel module (a template project)](https://musteresel.github.io/posts/2020/02/cmake-template-linux-kernel-module.html)

构建脚本这里就不粘贴了。具体见仓库。

#### 运行

```shell
# 安装上面编译生成的内核模块
insmod netlink_demo.ko

# 运行用户空间进程,可以看到下面输出
./netlink_user
Sent 29 bytes
==== LEN(31) TYPE(3) FLAGS(0) SEQ(0) PID(0)

Received data: Hello userspace
Received 32 bytes

# 查看内核输出
dmesg
[ 9169.846147] netlink_demo: loading out-of-tree module taints kernel.
[ 9169.846194] netlink_demo: module verification failed: signature and/or required key missing - tainting kernel
[ 9169.847580] Loading the netlink module
[ 9189.842531] ==== LEN(29) TYPE(0) FLAGS(0) SEQ(0) PORTID(1)
[ 9189.842536] Received 13 bytes: Hello kernel
```
---

## 最后

目前，我不需要写netlink的内核模块，只需要关注netlink在用户空间的使用即可。

通常，我应该也不会直接调用netlink API , 而是调用libnl API，它更高层一些。

唉，又得去看libnl的文档。可怜的程序员。


