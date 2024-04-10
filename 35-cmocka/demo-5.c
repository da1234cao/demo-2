// copy from:
// https://stackoverflow.com/questions/47039675/gccs-linker-wrap-will-not-wrap-over-static-library-function

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <stdio.h>

#include "cmocka.h"
#include "demo-5-lib.h"

void __wrap_world(void) { puts(mock_ptr_type(char *)); }

void test_hello(void **state) {
  (void)state; /* unused */

  assert_int_equal(7, 1 + 2 * 3);

  will_return(__wrap_world, "world from tests\n");
  hello();
}

const struct CMUnitTest mytests[] = {
    cmocka_unit_test(test_hello),
};

int main(void) { return cmocka_run_group_tests(mytests, NULL, NULL); }