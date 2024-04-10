cmake_minimum_required(VERSION 3.11)

project(cmocka-hello)

include(ExternalProject)

ExternalProject_Add(cmocka PREFIX cmocka
    URL ${CMAKE_CURRENT_SOURCE_DIR}/cmocka-1.1.7.tar.xz
    URL_MD5 dc23f4870eb7dfa6da065f3056e815b0
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

ExternalProject_Get_Property(cmocka INSTALL_DIR)
file(MAKE_DIRECTORY ${INSTALL_DIR}/include)

add_library(libcmocka STATIC IMPORTED GLOBAL)
add_dependencies(libcmocka cmocka)
set_target_properties(libcmocka PROPERTIES
    IMPORTED_LOCATION ${INSTALL_DIR}/lib64/libcmocka.a
    INTERFACE_LINK_DIRECTORIES ${INSTALL_DIR}/lib64
    INTERFACE_INCLUDE_DIRECTORIES ${INSTALL_DIR}/include
)

add_executable(demo-1 demo-1.c)
target_link_libraries(demo-1 PRIVATE libcmocka)

add_executable(demo-2 demo-2.c)
target_link_libraries(demo-2 PRIVATE libcmocka)

add_executable(demo-3 demo-3.c)
target_link_libraries(demo-3 PRIVATE libcmocka)

add_executable(demo-4-test demo-4-test.c)
target_link_libraries(demo-4-test PRIVATE libcmocka)
target_link_options(demo-4-test  PRIVATE -Wl,--wrap=get_value)

add_executable(demo-5 demo-5.c demo-5-lib.c)
target_link_libraries(demo-5 PRIVATE libcmocka)
target_link_options(demo-5  PRIVATE -Wl,--wrap=get_value)