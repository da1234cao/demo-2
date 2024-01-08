#include <syslog.h>

int main(int argc, char *argv[]) {
  openlog("demo2", LOG_CRON | LOG_PID, LOG_USER);
  syslog(LOG_DEBUG, "just use syslog.use openlog");
  closelog();
  return 0;
}