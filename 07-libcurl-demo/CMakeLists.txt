cmake_minimum_required(VERSION 3.1)
project(hello_curl)

find_package(CURL REQUIRED)

add_executable(https https.c)

message("curl include dir: ${CURL_INCLUDE_DIRS}")
message("curl lib : ${CURL_LIBRARIES}")
include_directories(${CURL_INCLUDE_DIRS})
target_link_libraries(https ${CURL_LIBRARIES})
