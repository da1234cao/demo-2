#include "net.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int socket_connect(connection *con) {
  int status;
  struct addrinfo hints;
  struct addrinfo *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = 0;
  hints.ai_family = AF_INET; // AF_UNSPEC为 AF_INET or AF_INET6
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP; // 指定协议为TCP

  char *host = copy_url_part(con->url, &con->url_parts, UF_HOST);
  char *schema = copy_url_part(con->url, &con->url_parts, UF_SCHEMA);
  if ((status = getaddrinfo(host, schema, &hints, &res)) != 0) {
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

  free(host);
  free(schema);
}

status socket_write(connection *con) {
  int n = 0;
  while (n < strlen(con->send_uf)) {
    int send_n = write(con->fd, con->send_uf + n, strlen(con->send_uf) - n);
    if (send_n == -1) {
      return ERROR;
    }
    n += send_n;
  }
  return OK;
}

status socket_read(connection *con) {
  while (con->is_recv_all == 0 && (sizeof(con->recv_buf) - con->recv_n > 0)) {
    // 同步方式，当读取到一个完整的response的时候，停止
    int n = read(con->fd, con->recv_buf + con->recv_n,
                 sizeof(con->recv_buf) - con->recv_n);
    con->recv_n += n;
    http_parser_execute(&con->parser, &con->settings, con->recv_buf,
                        con->recv_n);
  }
}

void construct_request(connection *con) {
  char *host = copy_url_part(con->url, &con->url_parts, UF_HOST);
  char *message_fmt = "GET / HTTP/1.0\r\n\r\n";
  snprintf(con->send_uf, SENDBUF - 1, message_fmt);
}