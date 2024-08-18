file(GLOB _SOURCES "src/${RUN_ARCH}/*.c" "src/${RUN_ARCH}/*.S")

include_directories(include)
include_directories(../main/src)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/arch)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/common)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/kernel)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/driver/spinlock)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/driver/rtos_cmdqu)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/driver/uart)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/hal/config)

add_library(comm STATIC ${_SOURCES})
install(TARGETS comm DESTINATION lib)

