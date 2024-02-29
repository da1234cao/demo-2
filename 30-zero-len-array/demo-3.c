#include <stdio.h>
#include <stdlib.h>

struct buffer {
  unsigned int len;
  long contents[];
};

int main(int argc, char *argv[]) {
  unsigned int buf_len = 100;

  // construct a buffer
  struct buffer *buffer =
      malloc(sizeof(struct buffer) + buf_len * (sizeof(long)));
  buffer->len = buf_len;

  free(buffer);
  return 0;
}