cmake_minimum_required(VERSION 3.11)

project(gtest-demo)

find_package(PkgConfig)
pkg_search_module(GTEST REQUIRED gtest_main)
pkg_search_module(GMOCK REQUIRED gmock_main)

add_executable(mock-demo mock-demo.cpp)

target_link_libraries(mock-demo PRIVATE ${GTEST_LDFLAGS} ${GMOCK_LIBRARIES})
target_compile_options(mock-demo PRIVATE ${GTEST_CFLAGS})

enable_testing()
add_test(NAME mock-demo COMMAND mock-demo)
