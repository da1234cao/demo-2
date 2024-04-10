/* test.c */

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <cmocka.h>

// ref:
// https://stackoverflow.com/questions/47039675/gccs-linker-wrap-will-not-wrap-over-static-library-function
// https://stackoverflow.com/questions/43183060/how-to-wrap-existing-function-in-c
// add_one() 和 get_value() 必须在不同的文件中，否则ld没有这个

#include "demo-4.c"

int __wrap_get_value() {
  printf("call __wrap_get_value\n");
  int v;
  v = mock_type(int);
  return v;
}

static void add_test(void **state) {
  (void)state;
  int a;

  will_return(__wrap_get_value, 3);

  a = add_one();
  assert_int_equal(a, 4);
}

int main(int argc, char *argv[]) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(add_test),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}