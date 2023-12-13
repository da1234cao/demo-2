cmake_minimum_required(VERSION 3.11)

project(dpdk_multi_process_msg)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDPDK REQUIRED libdpdk)
include_directories(${LIBDPDK_INCLUDE_DIRS})
link_directories(${LIBDPDK_LIBRARY_DIRS})
message(STATUS "LIBDPDK_LIBRARIES: ${LIBDPDK_LIBRARIES}")
message(STATUS "LIBDPDK_INCLUDE_DIRS: ${LIBDPDK_INCLUDE_DIRS}")
message(STATUS "LIBDPDK_LIBRARY_DIRS: ${LIBDPDK_LIBRARY_DIRS}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(primary primary.c)
target_link_libraries(primary PRIVATE ${LIBDPDK_LIBRARIES})

add_executable(secondary secondary.c)
target_link_libraries(secondary PRIVATE ${LIBDPDK_LIBRARIES})