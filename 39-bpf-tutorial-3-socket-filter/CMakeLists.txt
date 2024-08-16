cmake_minimum_required(VERSION 3.8)

project(bpf-tutorial-3-socket-filter VERSION 0.1.0 LANGUAGES C)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(BpfObject REQUIRED)

bpf_object(sockfilter sockfilter.bpf.c)

add_executable(sockfilter sockfilter.c)
target_link_libraries(sockfilter PRIVATE sockfilter_skel)