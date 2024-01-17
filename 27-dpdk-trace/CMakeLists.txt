cmake_minimum_required(VERSION 3.11)

project(dpdk_trace_test)

# arch的参数和编译dpdk时的cpu_instruction_set参数保持一致
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=corei7")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=corei7")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDPDK REQUIRED libdpdk)
include_directories(${LIBDPDK_STATIC_INCLUDE_DIRS})
link_directories(${LIBDPDK_STATIC_LIBRARY_DIRS})
# message(STATUS "LIBDPDK_STATIC_LIBRARIES: ${LIBDPDK_STATIC_LIBRARIES}")
# message(STATUS "LIBDPDK_STATIC_INCLUDE_DIRS: ${LIBDPDK_STATIC_INCLUDE_DIRS}")
# message(STATUS "LIBDPDK_STATIC_LIBRARY_DIRS: ${LIBDPDK_STATIC_LIBRARY_DIRS}")

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBDPDK_STATIC_LIBRARIES})