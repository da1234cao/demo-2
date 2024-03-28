cmake_minimum_required(VERSION 3.11)

project(kafka-hello)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

add_executable(consumer consumer.c)
target_include_directories(consumer PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(consumer PRIVATE rdkafka ${GLIB_LDFLAGS})

add_executable(producer producer.c)
target_include_directories(producer PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(producer PRIVATE rdkafka ${GLIB_LDFLAGS})
