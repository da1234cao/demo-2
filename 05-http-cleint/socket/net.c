#include "net.h"
#include "utils.h"
#include <netdb.h>
#include <stdio.h>

int socket_connect(connection *con) {
  int status;
  struct addrinfo hints;
  struct addrinfo *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // AF_UNSPEC为 AF_INET or AF_INET6
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP; // 指定协议为TCP

  if ((status = getaddrinfo(copy_url_part(con->url, &con->url_parts, UF_HOST),
                            NULL, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return -1;
  }

  for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
    int sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd == -1) {
      continue;
    }
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) != -1) {
      con->fd = sockfd;
      break;
    }
  }
}