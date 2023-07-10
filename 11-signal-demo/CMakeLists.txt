cmake_minimum_required(VERSION 3.11)

project(signal-test)

if(not WIN32)
  add_executable(linux-signal ${CMAKE_SOURCE_DIR}/linux-signal.cpp)
endif()

add_executable(std-signal ${CMAKE_SOURCE_DIR}/std-signal.cpp)
