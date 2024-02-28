#include <stdio.h>
#include <stdlib.h>

struct line {
  int length;
  char contents[0]; // sizeof(struct line) == 4
  // int contents[0]; // sizeof(struct line) == 4
  // long int contents[0]; // sizeof(struct line) == 8
};

int main(int argc, char *argv[]) {
  unsigned int len = 20;
  struct line *line = malloc(sizeof(struct line) + len);
  line->length = len;
}