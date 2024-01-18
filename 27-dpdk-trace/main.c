#include "trace_point.h"
#include <rte_eal.h>
#include <rte_trace.h>

int global_count = 0;

int main(int argc, char *argv[]) {

  if (rte_eal_init(argc, argv) < 0) {
    rte_exit(EXIT_FAILURE, "fail in init");
  }

  rte_trace_point_t *trace;

  trace = rte_trace_point_lookup("app.trace.string");
  if (trace == NULL) {
    printf("trace not found\n");
    return 1;
  }
  printf("trace is found\n");

  rte_trace_mode_set(RTE_TRACE_MODE_OVERWRITE);

  if (!rte_trace_point_is_enabled(&__app_trace_string)) {
    printf("trace not enable\n");
  } else {
    printf("trace is already allowed when registering\n");
  }

  rte_trace_point_enable(&__app_trace_string);
  if (rte_trace_point_is_enabled(&__app_trace_string)) {
    printf("trace is enable\n");
  }

  app_trace_string("hello world");

  // rte_trace_metadata_dump(stdout);
  // rte_trace_dump(stdout);

  if (rte_trace_save() < 0) {
    printf("fail to save trace to file.\n");
  }

  rte_eal_cleanup();

  return 0;
}