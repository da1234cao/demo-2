#include <syslog.h>

int main(int argc, char *argv[]) {
  openlog("demo", LOG_CRON | LOG_PID, LOG_USER);
  syslog(LOG_DEBUG, "just use syslog.use openlog");
  closelog();
  return 0;
}