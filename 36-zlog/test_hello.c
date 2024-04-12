#include <zlog.h>

#include <stdio.h>

#include "zlog.h"

int main(int argc, char **argv)

{

  int rc;
  zlog_category_t *c;

  rc = zlog_init("test_hello.conf");

  if (rc) {
    printf("init failed\n");
    return -1;
  }

  c = zlog_get_category("my_cat");

  if (!c) {
    printf("get cat fail\n");

    zlog_fini();
    return -2;
  }

  zlog_info(c, "hello, zlog");
  zlog_debug(c, "debug, zlog");
  zlog_warn(c, "warn, zlog");
  // zlog_error(c, "error zlog");

  zlog_fini();

  return 0;
}