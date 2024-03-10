#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  openlog("system", LOG_PERROR, 0);

  //   char *cmd = "ls -alh NO_EXIST_FILE 2>&1";
  char *cmd = "ls -alh NO_EXIST_FILE";

  FILE *fp = popen(cmd, "r");
  if (fp == NULL) {
    syslog(LOG_ERR, "create subprocess failed: %s", strerror(errno));
    exit(0);
  }

  char line[1024];
  while (fgets(line, sizeof(line), fp) != NULL) {
    printf("%s", line);
  }

  int status = pclose(fp);
  if (status == -1) {
    syslog(LOG_ERR, "pclose failed: %s", strerror(errno));
  } else if (WIFEXITED(status)) {
    int ret = WEXITSTATUS(status);
    syslog(LOG_INFO, "subprocess return code: %d", ret);
  }

  exit(0);
}