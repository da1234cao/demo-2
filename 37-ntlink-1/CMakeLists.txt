cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(driver VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find kernel headers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(KernelHeaders REQUIRED)

# kernel module name
set(MODULE_NAME netlink_demo)
set(MODULE_FILE ${MODULE_NAME}.ko)

set(MODULE_SRC netlink_kernel.c)

# Generate the Kbuild file through cmake.
foreach(SOURCE_FILE ${MODULE_SRC})
    get_filename_component(BASENAME ${SOURCE_FILE} NAME_WE)
    set(MODULE_OBJECT_FILE "${BASENAME}.o")
    list(APPEND MODULE_OBJECT_FILES ${MODULE_OBJECT_FILE})
endforeach()
string(REPLACE ";" " " OBJECT_FILES_STRING "${MODULE_OBJECT_FILES}")
configure_file(Kbuild.in ${CMAKE_CURRENT_SOURCE_DIR}/Kbuild @ONLY)

set(KBUILD_CMD $(MAKE) -C ${KERNELHEADERS_DIR} M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(OUTPUT ${MODULE_FILE}
        COMMAND ${KBUILD_CMD} modules
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${MODULE_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/Kbuild VERBATIM)

add_custom_target(module ALL DEPENDS ${MODULE_FILE})
add_custom_target(module-clean COMMAND ${module_cmd} clean)

add_executable(netlink_user netlink_user.c)

