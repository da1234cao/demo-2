#include "common.h"

int main(int argc, char *argv) {
  const char *pipe_path = "/tmp/my_fifo";

  int fd = create_named_pipe(pipe_path, O_WRONLY);
  if (fd < 0) {
    fprintf(stderr, "Error creating named pipe: %s\n", pipe_path);
    return -1;
  }

  char msg[] = "hello wrold";
  write(fd, msg, sizeof(msg));

  close(fd);
  return 0;
}