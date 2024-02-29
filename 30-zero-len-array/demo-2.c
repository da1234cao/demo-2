#include <stdio.h>
#include <stdlib.h>

struct buffer {
  unsigned int len;
  char *contents;
};

int main(int argc, char *argv[]) {
  unsigned int buf_len = 100;

  // construct a buffer
  struct buffer *buffer =
      malloc(sizeof(struct buffer) + buf_len * (sizeof(char)));
  buffer->contents = (char *)buffer + sizeof(struct buffer);
  buffer->len = buf_len;

  snprintf(buffer->contents, buffer->len, "%s", "hello world");
  printf("%s\n", buffer->contents);

  free(buffer);
  return 0;
}