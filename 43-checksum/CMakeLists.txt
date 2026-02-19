cmake_minimum_required(VERSION 3.10)
project(internet_checksum CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

add_executable(internet_checksum internet_checksum.cpp)
