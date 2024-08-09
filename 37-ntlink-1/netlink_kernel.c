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