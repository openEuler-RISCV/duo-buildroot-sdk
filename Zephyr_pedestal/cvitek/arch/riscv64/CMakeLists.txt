file(GLOB _SOURCES "src/*.c" "src/*.S")
file(GLOB _HEADERS "include/*.h")

include_directories(include)

add_library(arch STATIC ${_SOURCES})

install(TARGETS arch DESTINATION lib)
install(FILES ${_HEADERS} DESTINATION include/arch)