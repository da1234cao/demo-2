#include "main.h"
#include "net.h"
#include "connection.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  connection con = {};
  init_connection(&con);
  strncpy(con.url, "https://www.baidu.com", sizeof(con.url));
  http_parser_parse_url(con.url, strlen(con.url), 0, &con.url_parts);

  socket_connect(&con); // 建立连接
  construct_request(&con); // 构造一个request请求
  socket_write(&con); // 发送请求
  socket_read(&con); // 读取回复

  print_connection(&con);
  return 0;
}