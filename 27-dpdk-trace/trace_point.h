// clang-format off
#include <rte_trace_point_register.h>
#include <rte_trace_point.h>
#include <rte_trace.h>
// clang-format on

static inline void hello_trace() {
  static int count = 0;
  count++;
  printf("hello trace %d\n", count);
}

// clang-format off
RTE_TRACE_POINT(
       app_trace_string,
       RTE_TRACE_POINT_ARGS(const char *str),
       rte_trace_point_emit_string(str);
       hello_trace();
)
// clang-format on

RTE_TRACE_POINT_REGISTER(app_trace_string, app.trace.string)