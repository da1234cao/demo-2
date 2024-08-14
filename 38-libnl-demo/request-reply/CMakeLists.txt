cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(libnl-demo VERSION 0.1.0 LANGUAGES C)

find_package(PkgConfig)
pkg_check_modules(NL3 REQUIRED libnl-3.0 libnl-genl-3.0 libnl-route-3.0)

add_executable(demo-1 demo-1.c)
target_include_directories(demo-1 PRIVATE ${NL3_INCLUDE_DIRS})
target_link_libraries(demo-1 PRIVATE ${NL3_LIBRARIES})

add_executable(demo-2 demo-2.c)
target_include_directories(demo-2 PRIVATE ${NL3_INCLUDE_DIRS})
target_link_libraries(demo-2 PRIVATE ${NL3_LIBRARIES})

