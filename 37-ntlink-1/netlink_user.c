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