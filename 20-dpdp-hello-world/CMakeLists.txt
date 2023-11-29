cmake_minimum_required(VERSION 3.11)

project(dpdk_hello_world)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDPDK REQUIRED libdpdk)
include_directories(${LIBDPDK_INCLUDE_DIRS})
link_directories(${LIBDPDK_LIBRARY_DIRS})
message(STATUS "LIBDPDK_LIBRARIES: ${LIBDPDK_LIBRARIES}")
message(STATUS "LIBDPDK_INCLUDE_DIRS: ${LIBDPDK_INCLUDE_DIRS}")
message(STATUS "LIBDPDK_LIBRARY_DIRS: ${LIBDPDK_LIBRARY_DIRS}")

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBDPDK_LIBRARIES})