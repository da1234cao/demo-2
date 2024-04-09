
// frome: https://qianchenzhumeng.github.io/posts/cmocka_tutorial/

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <cmocka.h>

int setup(void **state) {
  printf("setup...\n");
  return 0;
}

int teardown(void **state) {
  printf("teardown...\n");
  return 0;
}

void test_case(void **state) { printf("test...\n"); }

int main(int argc, char *argv[]) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup_teardown(test_case, setup, teardown),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}