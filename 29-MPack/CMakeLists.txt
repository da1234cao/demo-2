cmake_minimum_required(VERSION 3.11)

project(mpacp_demo)

include(ExternalProject)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

# ExternalProject_Add(mpack PREFIX mpack
#     URL ${CMAKE_CURRENT_SOURCE_DIR}/mpack-amalgamation-1.1.1.tar.gz
#     URL_MD5 10e7377a58e033f645386ee3ffd0879a
#     CONFIGURE_COMMAND ""
#     BUILD_COMMAND ""
#     INSTALL_COMMAND "")
# ExternalProject_Get_Property(mpack SOURCE_DIR)
# set(MPACK_SOURCE ${SOURCE_DIR}/src/mpack/mpack.c)
# set(MPACK_HEADER_PATH ${SOURCE_DIR}/src/mpack)

set(MPACK_DIR ${CMAKE_BINARY_DIR}/mpack)
execute_process(
    COMMAND mkdir -p ${MPACK_DIR}
    COMMAND tar -zxf ${CMAKE_CURRENT_SOURCE_DIR}/mpack-amalgamation-1.1.1.tar.gz -C mpack --strip-components=1
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    RESULT_VARIABLE RC
    ERROR_VARIABLE ERR)

if(RC)
    message(FATAL_ERROR "Failed to unzip mpack: ${ERR}")
endif()

include_directories(${MPACK_DIR}/src/mpack)

add_executable(base base.c ${MPACK_DIR}/src/mpack/mpack.c)

add_executable(over-write over-write.c ${MPACK_DIR}/src/mpack/mpack.c)

add_executable(over-write-2 over-write-2.c ${MPACK_DIR}/src/mpack/mpack.c)

