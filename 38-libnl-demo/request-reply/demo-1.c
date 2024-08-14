#include <linux/rtnetlink.h>
#include <netlink/msg.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/socket.h>

int netlink_add(const char *iftype, const char *ifname) {
  int ret = 0;
  struct nl_msg *msg = NULL;
  struct nl_sock *sk = NULL;

  // 构建请求
  msg = nlmsg_alloc_simple(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_ACK |
                                            NLM_F_CREATE | NLM_F_EXCL);

  struct ifinfomsg ifi = {};
  ifi.ifi_family = AF_UNSPEC;

  ret = nlmsg_append(msg, &ifi, sizeof(ifi), NLMSG_ALIGNTO);
  if (ret < 0) {
    printf("%s", nl_geterror(ret));
    goto end;
  }

#if 0
  ret = nla_put_string(msg, IFLA_INFO_KIND, iftype);
  if (ret < 0) {
    goto end;
  }
#endif

  struct nlattr *info = nla_nest_start(msg, IFLA_LINKINFO);
  ret = nla_put_string(msg, IFLA_INFO_KIND, iftype);
  if (ret < 0) {
    printf("%s", nl_geterror(ret));
    goto end;
  }
  nla_nest_end(msg, info);

  ret = nla_put_string(msg, IFLA_IFNAME, ifname);
  if (ret < 0) {
    printf("%s", nl_geterror(ret));
    goto end;
  }

  // 创建套接字
  sk = nl_socket_alloc();

  nl_connect(sk, NETLINK_ROUTE);

  // 发送请求
  ret = nl_send_auto(sk, msg);
  if (ret < 0) {
    printf("%s", nl_geterror(ret));
    goto end;
  }

  // 接收回复
  ret = nl_recvmsgs_default(sk);
  if (ret < 0) {
    printf("%s", nl_geterror(ret));
    goto end;
  }

end:
  nlmsg_free(msg);
  nl_socket_free(sk);
  return 0;
}

int main(int argc, char *argv[]) { netlink_add("dummy", "dummy0"); }