#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  openlog("system", LOG_PERROR, 0);

  char *cmd = "ls -alh NO_EXIST_FILE";

  int status = system(cmd);
  if (status == -1) {
    syslog(LOG_ERR, "create subprocess failed: %s", strerror(errno));
    goto err;
  }

  if (WIFEXITED(status)) {
    int ret = WEXITSTATUS(status);
    syslog(LOG_INFO, "subprocess return code: %d", ret);
  }

err:
  exit(0);
}