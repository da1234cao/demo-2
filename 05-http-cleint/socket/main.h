#pragma once
#include "3rd/http_parser.h"
#include <openssl/ssl.h>

#define URL_MAX_LENGTH 1024
#define SENDBUF 1024
#define RECVBUF 8192

typedef enum { OK, ERROR, RETRY } status;

typedef struct connection {
  http_parser parser;
  int fd;
  SSL *ssl;
  char url[URL_MAX_LENGTH];
  struct http_parser_url url_parts;
  char send_uf[SENDBUF];
  int recv_n;
  char recv_buf[RECVBUF];
  int is_recv_all;
} connection;

struct sock {
  status (*connect)(connection *c);
  status (*read)(connection *c);
  status (*write)(connection *c);
  status (*close)(connection *c);
  status (*readable)(connection *c);
};