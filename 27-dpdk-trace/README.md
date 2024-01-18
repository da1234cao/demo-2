[toc]

# DPDK trace 使用

## 前言

日志用于记录不太频繁，比较高level的事情。trace记录频繁发生的事情，它的开销低。

trace可以在运行时，通过参数控制是否启用；可以在任何时间点，将trace记录的缓冲区保存到文件系统；支持overwrite和discard两种trace模式操作；支持字符串查找是否存在某个跟踪点；支持正则或者通配符启用trace；生成trace的内容格式为CTF。

本文参考：[Trace Library](https://doc.dpdk.org/guides/prog_guide/trace_lib.html)、 [dpdk/app/test/test\_trace.c at  DPDK/dpdk](https://github.com/DPDK/dpdk/blob/main/app/test/test_trace.c)

本文代码见仓库。

---

## trace的简单使用

首先是要创建跟踪点。RTE_TRACE_POINT这个宏展开还挺麻烦，我没有太搞清楚，自行查看源码。后面需要进行trace记录，则调用`app_trace_string(const char *str)`。

```c
// trace_point.h
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
```


接着，需要注册跟踪点。建议将下面内容放在c文件而不是头文件中。（能不能放在头文件中呢，不知道。DPDK在使用trace的时候是放在C文件中的。C语言的宏展开是烦人的，不方面调试。

```c
// trace_point_register.c
#include <rte_trace_point_register.h>

#include "trace_point.h"

RTE_TRACE_POINT_REGISTER(app_trace_string, app.trace.string)
```

接下来我们使用这个trace。代码比较简单：
- 查看是否存在该trace。
- 设置当跟踪缓冲区已满时，新的跟踪事件将覆盖跟踪缓冲区中现有的捕获事件。
- 查看这个trace是否已经允许。
- 在代码中允许该trace。
- 触发trace。
- 将trace缓冲区记录到文件系统中。

```c
// main.c
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
```

下面是构建代码。注意machine和ALLOW_EXPERIMENTAL_API，与DPDK构建时保持一致。

```cmake
cmake_minimum_required(VERSION 3.11)

project(dpdk_trace_test)

# arch的参数和编译dpdk时的cpu_instruction_set参数保持一致
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=corei7")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=corei7")

# 和dpdk-meson中保持一致
add_definitions(-DALLOW_EXPERIMENTAL_API)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDPDK REQUIRED libdpdk)
include_directories(${LIBDPDK_STATIC_INCLUDE_DIRS})
link_directories(${LIBDPDK_STATIC_LIBRARY_DIRS})

add_executable(${PROJECT_NAME}  trace_point_register.c main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBDPDK_STATIC_LIBRARIES})
```

输出如下。

```shell
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="YOUR_DPDK_INSTALL_PATH"
make

./dpdk_trace_test --trace-bufsz=2M --trace-dir=.
hello trace 1
EAL: Detected CPU lcores: 8
EAL: Detected NUMA nodes: 1
EAL: Detected static linkage of DPDK
EAL: Multi-process socket /var/run/dpdk/rte/mp_socket
EAL: Selected IOVA mode 'PA'
EAL: VFIO support initialized
TELEMETRY: No legacy callbacks, legacy socket not created
trace is found
trace not enable
trace is enable
hello trace 2
EAL: Trace dir: ./rte-2024-01-18-PM-10-20-04

# 查看trace
babeltrace rte-2024-01-18-PM-10-20-04 
[22:20:04.025673412] (+?.?????????) app.trace.string: { cpu_id = 0x0, name = "dpdk_trace_test" }, { str = "hello world" }
```