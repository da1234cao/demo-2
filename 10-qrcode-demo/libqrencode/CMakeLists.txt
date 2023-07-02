cmake_minimum_required(VERSION 3.11)

project(
  libqrencode-test
  VERSION 0.1
  LANGUAGES CXX)

# libqrencode
find_package(PkgConfig REQUIRED)
pkg_check_modules(QRENCODE REQUIRED libqrencode)
include_directories(${QRENCODE_INCLUDE_DIRS})
link_directories(${QRENCODE_LIBRARY_DIRS})

# png
add_definitions(-DHAVE_PNG=1)
find_package(PNG REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(libqrencode-test PRIVATE ${QRENCODE_LIBRARIES} PNG::PNG)
