# Install script for directory: /home/work/Zephyr_pedestal/cvitek/arch/riscv64

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/work/Zephyr_pedestal/cvitek/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/home/work/host-tools/gcc/riscv64-elf-x86_64/bin/riscv64-unknown-elf-objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/work/Zephyr_pedestal/cvitek/build/arch/riscv64/libarch.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/arch" TYPE FILE FILES
    "/home/work/Zephyr_pedestal/cvitek/arch/riscv64/include/arch_cpu.h"
    "/home/work/Zephyr_pedestal/cvitek/arch/riscv64/include/arch_helpers.h"
    "/home/work/Zephyr_pedestal/cvitek/arch/riscv64/include/arch_sleep.h"
    "/home/work/Zephyr_pedestal/cvitek/arch/riscv64/include/arch_time.h"
    "/home/work/Zephyr_pedestal/cvitek/arch/riscv64/include/core_rv64.h"
    "/home/work/Zephyr_pedestal/cvitek/arch/riscv64/include/csi_core.h"
    "/home/work/Zephyr_pedestal/cvitek/arch/riscv64/include/csi_rv64_gcc.h"
    "/home/work/Zephyr_pedestal/cvitek/arch/riscv64/include/csr.h"
    "/home/work/Zephyr_pedestal/cvitek/arch/riscv64/include/io.h"
    "/home/work/Zephyr_pedestal/cvitek/arch/riscv64/include/irq.h"
    "/home/work/Zephyr_pedestal/cvitek/arch/riscv64/include/ptrace.h"
    )
endif()

