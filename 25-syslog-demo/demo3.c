#include <syslog.h>

int main(int argc, char *argv[]) {
  openlog("demo3", LOG_CRON | LOG_PID, LOG_LOCAL6);
  syslog(LOG_DEBUG, "use local6 facility");
  closelog();
  return 0;
}