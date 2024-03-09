#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  openlog("exec", LOG_PERROR, 0);

  char *cmd = "ls";
  char *ls_argv[] = {"ls", "-alh", NULL};

  pid_t pid;
  if ((pid = fork()) < 0) {
    syslog(LOG_ERR, "fork error");
  } else if (pid == 0) { /* specify pathname, specify environment */
    if (execvp(cmd, ls_argv) < 0)
      syslog(LOG_ERR, "execvp error");
  }

  if (waitpid(pid, NULL, 0) < 0) {
    syslog(LOG_ERR, "wait error");
  }

  exit(0);
}