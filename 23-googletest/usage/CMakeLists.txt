cmake_minimum_required(VERSION 3.11)

project(gtest-demo)

find_package(PkgConfig)
pkg_search_module(GTEST REQUIRED gtest_main)

add_executable(demo-1 demo-1.cpp)

target_link_libraries(demo-1 PRIVATE ${GTEST_LDFLAGS})
target_compile_options(demo-1 PRIVATE ${GTEST_CFLAGS})

enable_testing()
add_test(NAME demo-1 COMMAND demo-1)
