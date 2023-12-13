#pragma once
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static inline int create_named_pipe(const char *path, int flag) {
  // 创建并打开一个命名管道
  int ret;
  ret = mkfifo(path, 0640);
  if (ret < 0 && ret == EEXIST) {
    perror("Executing mkfifo failed");
    return -1;
  }

  int fd = open(path, flag);
  if (fd < 0) {
    fprintf(stderr, "Error opening file: %s\n", path);
    return -1;
  }

  return fd;
}