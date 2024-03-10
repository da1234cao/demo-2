#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  openlog("exec", LOG_PERROR, 0);

  char *cmd = "ls";
  char *ls_argv[] = {"ls", "-alh", "NO_EXIST_FILE", NULL};

  pid_t pid;
  if ((pid = fork()) < 0) {
    syslog(LOG_ERR, "fork error");
  } else if (pid == 0) { /* specify pathname, specify environment */
    if (execvp(cmd, ls_argv) < 0) {
      syslog(LOG_ERR, "execvp error");
    }
  }

  int status;
  if (waitpid(pid, &status, 0) < 0) {
    syslog(LOG_ERR, "wait error");
  } else {
    if (WIFEXITED(status)) {
      int ret = WEXITSTATUS(status);
      syslog(LOG_INFO, "subprocess return code: %d", ret);
    }
  }

  exit(0);
}