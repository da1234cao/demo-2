cmake_minimum_required(VERSION 3.11)

project(zlog-hello)

include(ExternalProject)

ExternalProject_Add(zlog PREFIX zlog
    URL ${CMAKE_CURRENT_SOURCE_DIR}/1.2.17.tar.gz
    URL_MD5 d3ff9ba6cbafef89f98eb6a788ca2e78
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

ExternalProject_Get_Property(zlog INSTALL_DIR)
file(MAKE_DIRECTORY ${INSTALL_DIR}/include)

add_library(libzlog STATIC IMPORTED GLOBAL)
add_dependencies(libzlog zlog)
set_target_properties(libzlog PROPERTIES
    IMPORTED_LOCATION ${INSTALL_DIR}/lib/libzlog.a
    INTERFACE_LINK_DIRECTORIES ${INSTALL_DIR}/lib
    INTERFACE_INCLUDE_DIRECTORIES ${INSTALL_DIR}/include
    INTERFACE_LINK_LIBRARIES pthread
)


add_executable(test_hello test_hello.c)
target_link_libraries(test_hello libzlog)

add_custom_command(TARGET test_hello POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/test_hello.conf
            ${CMAKE_BINARY_DIR}/test_hello.conf
    COMMENT "copy config file to build directory"
)