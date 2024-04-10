#include "demo-5-lib.h"
#include <stdio.h>

// __attribute__((weak)) void world() { printf("world from lib\n"); }

void world() { printf("world from lib\n"); }

void hello() {
  printf("hello\n");
  world();
}