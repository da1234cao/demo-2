#include "connection.h"
#include <stdlib.h>
#include <string.h>

int on_message_complete_cb(http_parser *parser) {
  connection *con = (connection *)(parser->data);
  con->is_recv_all = 1;
  return 0;
}

void init_connection(connection *con) {
  memset(con, 0, sizeof(*con));
  http_parser_init(&con->parser, HTTP_RESPONSE);
  con->settings.on_message_complete = on_message_complete_cb;
  con->parser.data = con;
}

char *copy_url_part(const char *url, const struct http_parser_url *parts,
                    enum http_parser_url_fields field) {
  char *part = NULL;

  if (parts->field_set & (1 << field)) {
    uint16_t off = parts->field_data[field].off;
    uint16_t len = parts->field_data[field].len;
    part = (char *)calloc(1, len + 1 * sizeof(char));
    memcpy(part, &url[off], len);
  }

  return part;
}

void print_connection(const connection *con) {
  printf("url: %s\n", con->url);
  printf("UF_SCHEMA: %s\n",
         copy_url_part(con->url, &con->url_parts, UF_SCHEMA));
  printf("UF_HOST: %s\n", copy_url_part(con->url, &con->url_parts, UF_HOST));
  printf("request: %s\n", con->send_uf);
  printf("response: %s\n", con->recv_buf);
}

void construct_request(connection *con) {
  char *host = copy_url_part(con->url, &con->url_parts, UF_HOST);
  char *message_fmt = "GET / HTTP/1.0\r\nHost: %s\r\n\r\n";
  snprintf(con->send_uf, SENDBUF - 1, message_fmt, host);
}

int is_https(connection *con) {
  char *schema = copy_url_part(con->url, &con->url_parts, UF_SCHEMA);
  strcasecmp(schema, "https") == 0 ? 1 : -1;
}