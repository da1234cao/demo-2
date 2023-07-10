cmake_minimum_required(VERSION 3.11)

project(signal-test)

if(NOT WIN32)
  message(FATAL_ERROR "this file is only in windows")
endif()

add_executable(receiver ${CMAKE_SOURCE_DIR}/receiver.cpp)
add_executable(sender ${CMAKE_SOURCE_DIR}/sender.cpp)
