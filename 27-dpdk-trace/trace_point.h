#pragma once

// clang-format off
#include <rte_trace_point_register.h>
#include <rte_trace_point.h>
#include <rte_trace.h>
// clang-format on

extern int global_count;
static inline void hello_trace() {
  global_count++;
  printf("hello trace %d\n", global_count);
}

// clang-format off
RTE_TRACE_POINT(
       app_trace_string,
       RTE_TRACE_POINT_ARGS(const char *str),
       rte_trace_point_emit_string(str);
       hello_trace();
)
// clang-format on