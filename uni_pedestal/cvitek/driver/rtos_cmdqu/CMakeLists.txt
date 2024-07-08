file(GLOB _SOURCES "src/*.c")
file(GLOB _HEADERS "include/*.h")

include_directories(include)
include_directories(${DRIVER_SPINLOCK_DIR}/include)
include_directories(${DRIVER_UART_DIR}/include)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/arch)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/common)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/hal/config)
add_library(rtos_cmdqu OBJECT ${_SOURCES})

install(FILES ${_HEADERS} DESTINATION include/driver/rtos_cmdqu)
