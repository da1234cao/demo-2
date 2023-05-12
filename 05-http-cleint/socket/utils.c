#include "utils.h"
#include "3rd/http_parser.h"
#include <stdlib.h>
#include <string.h>

void init_connection(connection *con){
  memset(con, 0, siezof(*con));
  http_parser_init(&con->parser, HTTP_RESPONSE);
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
}