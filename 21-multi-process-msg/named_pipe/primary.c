#include "common.h"

int main(int argc, char *argv) {
  const char *pipe_path = "/tmp/my_fifo";

  int fd = create_named_pipe(pipe_path, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Error creating named pipe: %s\n", pipe_path);
    return -1;
  }

  char msg[256];
  while (read(fd, msg, sizeof(msg))) {
    printf("%s\n", msg);
  }

  close(fd);
  return 0;
}