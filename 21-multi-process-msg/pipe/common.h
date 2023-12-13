#pragma once
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static inline int get_process_name(char *buffer, size_t size) {
  // 获取当前进程的ID
  pid_t pid = getpid();

  // 构建/proc/[pid]/comm的路径
  char proc_path[256];
  snprintf(proc_path, sizeof(proc_path), "/proc/%d/comm", pid);

  // 打开文件
  FILE *file = fopen(proc_path, "r");
  if (file == NULL) {
    perror("fopen");
    return -1; // 失败时返回-1
  }

  // 读取文件内容（进程名）
  if (fgets(buffer, size, file) == NULL) {
    perror("fgets");
    fclose(file);
    return -1; // 失败时返回-1
  }

  // 去掉末尾的换行符
  size_t length = strlen(buffer);
  if (length > 0 && buffer[length - 1] == '\n') {
    buffer[length - 1] = '\0';
  }

  // 关闭文件
  fclose(file);

  return 0; // 成功时返回0
}