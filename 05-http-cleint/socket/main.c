#include "main.h"
#include "connection.h"
#include "net.h"
#include "ssl.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  connection con = {};
  init_connection(&con);
  strncpy(con.url, "https://www.baidu.com", sizeof(con.url));
  http_parser_parse_url(con.url, strlen(con.url), 0, &con.url_parts);

  construct_request(&con); // 构造一个request请求

  struct sock con_sock = {.init = ssl_init,
                          .connect = ssl_connect,
                          .write = ssl_write,
                          .read = ssl_read,
                          .readable = ssl_readable,
                          .close = ssl_close};

  if (is_https(&con)) {
    con_sock.init = socket_init;
    con_sock.connect = socket_connect;
    con_sock.write = socket_write;
    con_sock.read = socket_read;
    con_sock.readable = sock_readable;
    con_sock.close = socket_close;
  }

  con_sock.init(&con);    // ssl需要初始化
  con_sock.connect(&con); // 建立连接
  con_sock.write(&con);   // 发送请求
  con_sock.read(&con);    // 读取回复
  con_sock.close(&con);   // 关闭连接

  print_connection(&con); // 打印连接结构体中的内容
  return 0;
}