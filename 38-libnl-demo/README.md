[toc]

# libnl教程(1):订阅内核的通知

## 前言

我之前整理过：[netlink 简介](https://blog.csdn.net/sinat_38816924/article/details/141069359)。

`netlink` 是 [libnl](https://www.infradead.org/~tgr/libnl/) 的基础。

在开始之前，需要先翻看一遍官方文档：[Netlink Library (libnl)](https://www.infradead.org/~tgr/libnl/doc/core.html) 、[Routing Family Netlink Library (libnl-route)](https://www.infradead.org/~tgr/libnl/doc/route.html)。

先翻看一遍，混个眼熟。翻看过之后，我还是不会用，因为没有示例将一组API串起来使用。

下面我使用示例的方式，写 `libnl` 教程。一些内容摘录在上面链接，不一一标注出处。

## 目标

订阅路由子系统的通知。即，当网络链路发生变化、路由表更改时，用户层可以收到通知。

## netlink kernel multicast notifications

路由子系统的广播通知，是netlink kernel multicast notifications的一种。

按照位置划分，`netlink` 有三种通信方式：(1)内核向用户空间发送消息；(2)用户空间向内核发送消息；(3)用户空间向用户空间发送消息。

按照交互划分，`netlink` 有两种通信方式：(1)用户空间向内核发送请求(request), 内核向用户空间发送回复(reply); (2)用户空间的进程订阅感兴趣的广播组(multicast group), 内核主动给用户空间的进程发送广播，用户空间的进程无需回复。

本文介绍的是，用户空间订阅内核的广播通知(notifications)。

## 订阅内核的链路(link)变化通知

注：在网络中，通常使用几个术语来指代网络设备。虽然它们的含义不同，但过去它们可以互换使用。在 Linux 内核中，通常使用术语 `network device` 或 `netdev` 。在用户空间中，术语 `network interface`非常常见。路由 netlink 协议使用术语 `link`，`iproute2` 实用程序和大多数路由守护程序也是如此。

### 示例代码

示例代码修改提取自：https://github.com/FDio/vpp/blob/master/src/plugins/linux-cp/lcp_nl.c

```c
#include <netlink/msg.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/socket.h>

static void nl_link_add_dump(struct rtnl_link *link, void *arg) {
  printf("new link: %s\n", rtnl_link_get_name(link));
  printf("mtu: %d\n", rtnl_link_get_mtu(link));
}

static void nl_link_del_dump(struct rtnl_link *link, void *arg) {
  printf("delete link: %s\n", rtnl_link_get_name(link));
  printf("mtu: %d\n", rtnl_link_get_mtu(link));
}

static void nl_route_dispatch(struct nl_object *obj, void *arg) {
  int msgtype = nl_object_get_msgtype(obj);
  switch (msgtype) {
  case RTM_NEWLINK:
    nl_link_add_dump((struct rtnl_link *)obj, arg);
    break;
  case RTM_DELLINK:
    nl_link_del_dump((struct rtnl_link *)obj, arg);
    break;
  default:
    printf("unhandled: %s\n", nl_object_get_type(obj));
    break;
  }
}

/*
 * This function will be called for each valid netlink message received
 * in nl_recvmsgs_default()
 */
static int nl_route_cb(struct nl_msg *msg, void *arg) {
  nl_msg_parse(msg, nl_route_dispatch, NULL);
  return NL_OK;
}

int main(int argc, char *argv[]) {

  int ret = 0;

  /* Allocate a new socket */
  struct nl_sock *sk_route = nl_socket_alloc();
  if (sk_route == NULL) {
    printf("%s\n", nl_geterror(ret));
    exit(EXIT_FAILURE);
  }

  //  Notifications do not use sequence numbers, disable sequence number
  //  checking.
  nl_socket_disable_seq_check(sk_route);

  /*
   * Define a callback function, which will be called for each notification
   * received
   */
  nl_socket_modify_cb(sk_route, NL_CB_VALID, NL_CB_CUSTOM, nl_route_cb, NULL);

  /* Connect to routing netlink protocol */
  nl_connect(sk_route, NETLINK_ROUTE);

  /* must after nl_connect()
   * Subscribe to link notifications group */
  ret = nl_socket_add_memberships(
      sk_route, RTNLGRP_LINK, RTNLGRP_IPV6_IFADDR, RTNLGRP_IPV4_IFADDR,
      RTNLGRP_IPV4_ROUTE, RTNLGRP_IPV6_ROUTE, RTNLGRP_NEIGH, RTNLGRP_NOTIFY,
#ifdef RTNLGRP_MPLS_ROUTE /* not defined on CentOS/RHEL 7 */
      RTNLGRP_MPLS_ROUTE,
#endif
      RTNLGRP_IPV4_RULE, RTNLGRP_IPV6_RULE, NFNLGRP_NONE);

  /*
   * Start receiving messages. The function nl_recvmsgs_default() will block
   * until one or more netlink messages (notification) are received which
   * will be passed on to my_func().
   */

  while (1) {
    ret = nl_recvmsgs_default(sk_route);
    if (ret != 0) {
      printf("nl_recvmsgs_default failed: %s", nl_geterror(ret));
    }
  }
}
```

运行输出如下。

```shell
# 当使用ip命令操作链路时
ip link add name dummy0 type dummy
ip addr add 192.168.1.10/24 dev dummy0
ip link delete dummy0

# 该用户层程序，可以感知到链路的变化
new link: dummy0
mtu: 1500
unhandled: route/addr
unhandled: route/route
unhandled: route/addr
unhandled: route/route
delete link: dummy0
mtu: 1500
```

### 函数使用

- `nl_socket_alloc()`: 创建一个新的netlink socket。(其实这内部还没有创建socket fd)。
- `nl_socket_disable_seq_check()`: 在libnl中, Sequence Numbers 被封装起来，通常被不需要我们关注。它用于请求/响应模型，在通知模型中，必须禁用。
- `nl_socket_modify_cb()`: 设置回调函数。上面设置了一个自定义的回调函数。当收到消息，并且验证这个消息有效时，自动自动该回调函数。
- `nl_connect()`: 每个netlink协议都使用自己的协议号。链接配置接口是NETLINK_ROUTE协议系列的一部分。上面连接到路由netlink协议。
- `nl_socket_add_memberships()`: 订阅感兴趣的notifications group。例如，对于RTNLGRP_LINK广播组，可以收到添加/删除链路的通知。
- `nl_recvmsgs_default()`: 接收netlink消息的最简单的调用方法。它会接收和解析netlink消息，并调用回调函数。

### 难点问题

接收到通知的消息并不难。难处在于如何解析。

上面代码中使用了 `nl_msg_parse()` 函数，调用了回调函数。看里面的源码，可以看到其内部根据消息类型进行自动解析，内部调用的是`link_msg_parser()`函数进行解析。

如果手动解析，怎么解析呢？

参见官方文档可知，我们得调用 `nlmsg_parse()`函数。对于link 消息来说，它的解构如下。

![alt text](image.png)

我也不会解析，有些复杂。这里有个简单的示例：[c++ - not getting RTM_DELLINK netlink event - Stack Overflow](https://stackoverflow.com/questions/67424466/not-getting-rtm-dellink-netlink-event)