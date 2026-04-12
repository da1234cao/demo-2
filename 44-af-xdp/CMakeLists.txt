cmake_minimum_required(VERSION 3.20)

project(af-xdp)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Option to use system libbpf and libxdp or build from source
option(USE_SYSTEM_LIBBPF_XDP "Use system-installed libbpf and libxdp" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBBPF REQUIRED IMPORTED_TARGET libbpf)

if(USE_SYSTEM_LIBBPF_XDP)
    pkg_check_modules(LIBXDP REQUIRED IMPORTED_TARGET libxdp)
else()
    # Fetch and build xdp-tools from source
    include(FetchContent)
    FetchContent_Declare(
        xdp-tools
        GIT_REPOSITORY https://gitee.com/da1234cao/xdp-tools.git
        GIT_TAG v1.5.5
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        GIT_SUBMODULES ""
    )
    FetchContent_MakeAvailable(xdp-tools)
    
    # Build xdp-tools using autotools
    set(XDP_TOOLS_SOURCE_DIR ${xdp-tools_SOURCE_DIR})
    set(XDP_TOOLS_BINARY_DIR ${xdp-tools_BINARY_DIR})
    
    # Configure step - force building libbpf from source
    execute_process(
        COMMAND ./configure CFLAGS="-g -O0"
        WORKING_DIRECTORY ${XDP_TOOLS_SOURCE_DIR}
        RESULT_VARIABLE XDP_CONFIGURE_RESULT
    )
    if(NOT XDP_CONFIGURE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to configure xdp-tools")
    endif()
    
    # Build step
    execute_process(
        COMMAND make -j4
        WORKING_DIRECTORY ${XDP_TOOLS_SOURCE_DIR}
        RESULT_VARIABLE XDP_BUILD_RESULT
    )
    if(NOT XDP_BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to build xdp-tools")
    endif()
    
    # Set library paths
    set(LIBXDP_LIBRARY ${XDP_TOOLS_SOURCE_DIR}/lib/libxdp/libxdp.a)
    set(LIBXDP_INCLUDE_DIR ${XDP_TOOLS_SOURCE_DIR}/lib/libxdp)
    
    # Create imported targets
    add_library(libxdp STATIC IMPORTED)
    set_target_properties(libxdp PROPERTIES
        IMPORTED_LOCATION ${LIBXDP_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${LIBXDP_INCLUDE_DIR}
        INTERFACE_LINK_LIBRARIES "-lelf -lz"
    )
    
    set(LIBXDP libxdp)
endif()

# build bpf object
find_package(BpfObject REQUIRED)
bpf_object(af_xdp_kern af_xdp_kern.c)
add_custom_target(bpf_all ALL)
add_dependencies(bpf_all af_xdp_kern_bpfobj)

# fetch argparse
include(FetchContent)
set(FETCHCONTENT_QUIET OFF CACHE BOOL "Disable quiet mode for FetchContent" FORCE)

FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG          v3.2
    GIT_SHALLOW      TRUE
    GIT_PROGRESS    TRUE
    GIT_SUBMODULES ""
)
FetchContent_MakeAvailable(argparse)

# build main executable
add_executable(${PROJECT_NAME} main.cpp)
if(USE_SYSTEM_LIBBPF_XDP)
    target_link_libraries(${PROJECT_NAME} PRIVATE argparse PkgConfig::LIBXDP PkgConfig::LIBBPF)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE argparse ${LIBXDP} PkgConfig::LIBBPF)
endif()

