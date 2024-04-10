#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <cmocka.h>

// change to __real_get_value
int get_value() {
  printf("call get_value\n");
  return 1;
}