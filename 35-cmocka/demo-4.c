/* test.c */

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <cmocka.h>

// add_one() 和 get_value() 必须在不同的文件中，否则ld没有这个

extern int get_value();

int add_one() {
  int v;
  v = get_value(); // __wrap_get_value
  return v + 1;
}