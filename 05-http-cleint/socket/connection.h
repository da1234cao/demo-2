#pragma once
#include "3rd/http_parser.h"
#include <openssl/ssl.h>

#define URL_MAX_LENGTH 1024
#define SENDBUF 1024
#define RECVBUF 8192

typedef enum { OK, ERROR, RETRY } status;

typedef struct connection {
  http_parser parser;
  http_parser_settings settings;
  int fd;
  SSL_CTX *ctx;
  SSL *ssl;
  char url[URL_MAX_LENGTH];
  struct http_parser_url url_parts;
  char send_uf[SENDBUF];
  char recv_buf[RECVBUF];
  int recv_n;
  int is_recv_all;
} connection;

int on_message_complete_cb(http_parser *parser);

void init_connection(connection *con);

char *copy_url_part(const char *url, const struct http_parser_url *parts,
                    enum http_parser_url_fields field);

void print_connection(const connection *con);

void construct_request(connection *con);