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
    printf("%s\n", "nl_socket_alloc failed");
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