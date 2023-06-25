cmake_minimum_required(VERSION 3.1)
project(hello_protobuf)

set(Protobuf_USE_STATIC_LIBS ON)
find_package(Protobuf REQUIRED)
message("Protobuf_VERSION: ${Protobuf_VERSION}")
message("Protobuf_INCLUDE_DIRS: ${Protobuf_INCLUDE_DIRS}")
message("Protobuf_LIBRARIES: ${Protobuf_LIBRARIES}")

include_directories(${Protobuf_INCLUDE_DIRS})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_SOURCE_DIR}/addressbook.proto)
message("PROTO_SRCS: ${PROTO_SRCS}")
message("PROTO_HDRS: ${PROTO_HDRS}")

# ref: https://stackoverflow.com/questions/20824194/cmake-with-google-protocol-buffers
# protobuf_generate_cpp 生成的文件在build目录下
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(write_message write_message.cpp ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(read_message read_message.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(write_message ${Protobuf_LIBRARIES})
target_link_libraries(read_message ${Protobuf_LIBRARIES})