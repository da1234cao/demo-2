# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
# https://github.com/libbpf/libbpf-bootstrap/blob/master/tools/cmake/FindBpfObject.cmake
#[=======================================================================[.rst:
FindBpfObject
-------------

Minimal dependency finder + helper macro to build an eBPF object.

This module sets:

  BpfObject_FOUND

And provides:

  bpf_object(<name> <file.bpf.c> [<dep_header> ...])

The macro produces:
  - <name>.bpf.o
  - a custom target: <name>_bpfobj (depends on the .o)

#]=======================================================================]

if(NOT BPFOBJECT_BPFTOOL_EXE)
  find_program(BPFOBJECT_BPFTOOL_EXE NAMES bpftool DOC "Path to bpftool executable")
endif()

if(NOT BPFOBJECT_CLANG_EXE)
  find_program(BPFOBJECT_CLANG_EXE NAMES clang DOC "Path to clang executable")
endif()

if(BPFOBJECT_VMLINUX_H)
  get_filename_component(GENERATED_VMLINUX_DIR ${BPFOBJECT_VMLINUX_H} DIRECTORY)
elseif(BPFOBJECT_BPFTOOL_EXE)
  set(GENERATED_VMLINUX_DIR ${CMAKE_BINARY_DIR})
  set(BPFOBJECT_VMLINUX_H ${GENERATED_VMLINUX_DIR}/vmlinux.h)
  execute_process(
    COMMAND ${BPFOBJECT_BPFTOOL_EXE} btf dump file /sys/kernel/btf/vmlinux format c
    OUTPUT_FILE ${BPFOBJECT_VMLINUX_H}
    ERROR_VARIABLE VMLINUX_error
    RESULT_VARIABLE VMLINUX_result)
  if(NOT ${VMLINUX_result} EQUAL 0)
    message(FATAL_ERROR "Failed to dump vmlinux.h from BTF: ${VMLINUX_error}")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BpfObject
  REQUIRED_VARS
    BPFOBJECT_BPFTOOL_EXE
    BPFOBJECT_CLANG_EXE
    GENERATED_VMLINUX_DIR)

# Target arch macro for CO-RE (matches libbpf/bpftool conventions).
execute_process(
  COMMAND uname -m
  COMMAND sed -e "s/x86_64/x86/" -e "s/aarch64/arm64/" -e "s/ppc64le/powerpc/" -e "s/mips.*/mips/" -e "s/riscv64/riscv/"
  OUTPUT_VARIABLE ARCH_output
  ERROR_VARIABLE ARCH_error
  RESULT_VARIABLE ARCH_result
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT ${ARCH_result} EQUAL 0)
  message(FATAL_ERROR "Failed to determine target architecture:\n${ARCH_error}")
endif()
set(ARCH ${ARCH_output})

macro(bpf_object name input)
  set(BPF_C_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${input})

  set(BPF_H_FILES "")
  foreach(arg ${ARGN})
    list(APPEND BPF_H_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
  endforeach()

  set(BPF_O_FILE ${CMAKE_BINARY_DIR}/${name}.bpf.o)
  set(OUTPUT_TARGET ${name}_bpfobj)

  add_custom_command(
    OUTPUT ${BPF_O_FILE}
    COMMAND ${BPFOBJECT_CLANG_EXE} -g -O2 -target bpf -D__TARGET_ARCH_${ARCH}
            -I${GENERATED_VMLINUX_DIR}
            -c ${BPF_C_FILE} -o ${BPF_O_FILE}
    COMMAND_EXPAND_LISTS
    VERBATIM
    DEPENDS ${BPF_C_FILE} ${BPF_H_FILES}
    COMMENT "[clang] Building BPF object: ${name}")

  add_custom_target(${OUTPUT_TARGET} DEPENDS ${BPF_O_FILE})

  # Expose the built object path to callers if they want it.
  # set(${name}_BPF_O ${BPF_O_FILE} PARENT_SCOPE)
endmacro()

