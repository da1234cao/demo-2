cmake_minimum_required(VERSION 3.11)
project(hello_grpc)

set(Protobuf_USE_STATIC_LIBS ON)
find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++)

# Proto file
set(calcu_proto "${CMAKE_SOURCE_DIR}/calculator.proto")
get_filename_component(calcu_proto_path "${calcu_proto}" PATH)

set(calcu_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/calculator.pb.cc")
set(calcu_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/calculator.pb.h")
set(calcu_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/calculator.grpc.pb.cc")
set(calcu_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/calculator.grpc.pb.h")

# 不要使用execute_process(),它在配置的时候调用
# 使用add_custom_command(),当有target使用OUTPUT中的输出时,将调用这个命令重新生成
add_custom_command(
    OUTPUT "${calcu_proto_srcs}" "${calcu_proto_hdrs}" "${calcu_grpc_srcs}" "${calcu_grpc_hdrs}"
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
    -I "${calcu_proto_path}"
    --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
    "${calcu_proto}"
    DEPENDS "${calcu_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(calcu_grpc_proto
    ${calcu_proto_srcs}
    ${calcu_proto_hdrs}
    ${calcu_grpc_srcs}
    ${calcu_grpc_hdrs})

target_link_libraries(calcu_grpc_proto ${GRPC_LIBRARIES} ${Protobuf_LIBRARIES})

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} ${calcu_grpc_proto} ${Protobuf_LIBRARIES})

foreach(_target main client server)
    add_executable(${_target} "${CMAKE_SOURCE_DIR}/${_target}.cpp")
    target_link_libraries(${_target}
        calcu_grpc_proto ${GRPC_LIBRARIES} ${Protobuf_LIBRARIES})
endforeach()