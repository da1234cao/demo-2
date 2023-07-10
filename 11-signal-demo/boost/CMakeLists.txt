cmake_minimum_required(VERSION 3.11)

project(asio-signal-test)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED)

include_directories((${Boost_INCLUDE_DIRS}))

add_executable(asio-signal-test ${CMAKE_SOURCE_DIR}/boost-asio-signal.cpp)
target_link_libraries(asio-signal-test PRIVATE ${Boost_LIBRARIES})
