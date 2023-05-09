#include "main.h"
#include "net.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  connection con = {};
  strncpy(con.url, "https://www.baidu.com", sizeof(con.url));
  http_parser_parse_url(con.url, strlen(con.url), 0, &con.url_parts);

  socket_connect(&con);

  print_connection(&con);
  return 0;
}