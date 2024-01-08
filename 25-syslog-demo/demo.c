#include <syslog.h>

int main(int argc, char *argv[]) {
  // syslog(LOG_DEBUG| LOG_USER, "just use syslog.");
  syslog(LOG_DEBUG, "just use syslog.");
  return 0;
}