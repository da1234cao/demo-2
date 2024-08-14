#include <linux/rtnetlink.h>
#include <netlink/msg.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/socket.h>

int netlink_add(const char *iftype, const char *ifname) {
  struct rtnl_link *link = rtnl_link_alloc();
  rtnl_link_set_type(link, iftype);
  rtnl_link_set_name(link, ifname);

  struct nl_sock *sk = nl_socket_alloc();
  nl_connect(sk, NETLINK_ROUTE);

  rtnl_link_add(sk, link, NLM_F_CREATE | NLM_F_EXCL);
  return 0;
}

int main(int argc, char *argv[]) { netlink_add("dummy", "dummy0"); }