#pragma once

#include <rte_trace_point.h>

extern int global_count;
static inline void hello_trace() {
  global_count++;
  printf("hello trace %d\n", global_count);
}

// clang-format off
extern int global_count;
RTE_TRACE_POINT(
      app_trace_string,
      RTE_TRACE_POINT_ARGS(const char *str),
      rte_trace_point_emit_string(str);
      hello_trace();
)
// clang-format on