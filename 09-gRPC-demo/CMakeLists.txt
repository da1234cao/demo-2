cmake_minimum_required(VERSION 3.11)
project(hello_grpc)

set(Protobuf_USE_STATIC_LIBS ON)
# cmake-format: off
# ubuntu通过apt包管理器安装的grpc没有findgrpc.cmake这样的文件，但是有.pc这样的文件
# windows通过vcpkg库管理器安装的grpc有gRPCConfig.cmake,但是没有.pc文件
# 。。无语，大无语。。
# Linux上用PkgConfig获取库;windows上直接通过find_package查找;
# cmake-format: on
find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)
if(WIN32)
  find_package(GRPC REQUIRED)
  get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
  set(GRPC_LIBRARIES gRPC::grpc++) # 这样也可以,nb
else()
  find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
  pkg_check_modules(GRPC REQUIRED grpc++)
endif()

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
  OUTPUT "${calcu_proto_srcs}" "${calcu_proto_hdrs}" "${calcu_grpc_srcs}"
         "${calcu_grpc_hdrs}"
  COMMAND
    ${Protobuf_PROTOC_EXECUTABLE} ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}" -I "${calcu_proto_path}"
    --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}" "${calcu_proto}"
  DEPENDS "${calcu_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(calcu_grpc_proto ${calcu_proto_srcs} ${calcu_proto_hdrs}
                             ${calcu_grpc_srcs} ${calcu_grpc_hdrs})

target_link_libraries(calcu_grpc_proto ${GRPC_LIBRARIES} ${Protobuf_LIBRARIES})

foreach(_target main client server)
  add_executable(${_target} "${CMAKE_SOURCE_DIR}/${_target}.cpp")
  target_link_libraries(${_target} calcu_grpc_proto ${GRPC_LIBRARIES}
                        ${Protobuf_LIBRARIES})
endforeach()
