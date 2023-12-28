#include <inttypes.h>
#include <stdio.h>

struct internet {
  __uint16_t ip;
};

__uint8_t address[10];

int main(int argc, char *argv[]) {
  address[0] = 1;
  address[1] = 2;

  struct internet *net = (struct internet *)address;
  __uint16_t ip = net->ip;

  printf("%" PRIu8 "\n", address[0]);
  printf("%" PRIu8 "\n", address[1]);
  printf("%" PRIu16 "\n", ip);
}