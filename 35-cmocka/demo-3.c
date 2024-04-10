/* main.c */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

#include <cmocka.h>

static int setup(void **state) {
  int *answer = malloc(sizeof(int));

  assert_non_null(answer);
  *answer = 42;
  *state = answer;

  return 0;
}

static int teardown(void **state) {
  free(*state);

  return 0;
}

static void int_test_success(void **state) {
  int *answer = *state;

  assert_int_equal(*answer, 42);
}

int main(int argc, char *argv[]) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup_teardown(int_test_success, setup, teardown),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}