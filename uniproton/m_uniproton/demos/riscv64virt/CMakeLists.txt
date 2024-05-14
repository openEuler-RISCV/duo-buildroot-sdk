cmake_minimum_required(VERSION 3.12)
project(exmaples LANGUAGES C ASM)

set(HOME_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../..)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/bin/riscv64-unknown-elf-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/bin/riscv64-unknown-elf-g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PATH}/bin/riscv64-unknown-elf-gcc)
set(CMAKE_LINKER ${TOOLCHAIN_PATH}/bin/riscv64-unknown-linux-elf-ld)
set(CC_OPTION "-Wno-implicit-fallthrough -Wno-unused-variable -Wno-unused-but-set-variable -nostartfiles -g -march=rv64gc -mabi=lp64d -Wall -Werror  -Wextra -Wformat=2 -Wfloat-equal -Wshadow -Wtrampolines -O0 -Wdate-time  -static  -fno-PIE -fno-dwarf2-cfi-asm   -fomit-frame-pointer -fzero-initialized-in-bss -fdollars-in-identifiers -ffunction-sections -fdata-sections -fno-common -fno-aggressive-loop-optimizations -fno-optimize-strlen -fno-schedule-insns  -fno-inline-small-functions -fno-inline-functions-called-once -fno-strict-aliasing  -fno-stack-protector  -funsigned-char -fno-PIC -std=c99 -ffreestanding -Wno-undef -Wno-strict-prototypes -Wno-redundant-decls -Wno-cast-qual -Wno-parentheses -Wno-pointer-sign -Wno-unused-parameter -Wno-sign-compare -Wno-jump-misses-init -Wno-old-style-definition -Wno-shadow -Wno-unused-macros -mcmodel=medany" )
set(AS_OPTION "-g -march=rv64gc -mabi=lp64d -mcmodel=medany -static -fno-builtin -fno-PIE -Wall -fomit-frame-pointer -O0  -fzero-initialized-in-bss -fno-common ")
set(LD_OPTION "-static -no-pie -Wl,--wrap=memset -Wl,--wrap=memcpy")
set(CMAKE_C_FLAGS "${CC_OPTION}")
set(CMAKE_ASM_FLAGS "${AS_OPTION}")

set(CMAKE_LINK_FLAGS "${LD_OPTION} -T ${CMAKE_CURRENT_SOURCE_DIR}/build/rv64virt.ld")
set(CMAKE_EXE_LINKER_FLAGS "${LD_OPTION} -T ${CMAKE_CURRENT_SOURCE_DIR}/build/rv64virt.ld")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hw/riscv64
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/config/rv64virt
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/driver
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/misc_m
)


link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs
)

link_libraries(
    -Wl,--start-group
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/libRV64VIRT.a"
    -Wl,--end-group
)

add_subdirectory(bsp)
add_subdirectory(config)
add_subdirectory(apps)

list(APPEND OBJS $<TARGET_OBJECTS:bsp> $<TARGET_OBJECTS:config> $<TARGET_OBJECTS:app>)
add_executable(${APP} ${OBJS})
