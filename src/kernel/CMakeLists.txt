project(kernel LANGUAGES CXX C ASM)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp" "*.S")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "*.h")

include(FetchContent)

FetchContent_Declare(
        flanterm
        GIT_REPOSITORY https://github.com/Mintsuki/Flanterm.git
        GIT_TAG        trunk
)

FetchContent_MakeAvailable(flanterm)

file(GLOB_RECURSE FLANTERM_SOURCES CONFIGURE_DEPENDS ${flanterm_SOURCE_DIR}/src/*.c)

add_library(kernel.api INTERFACE)
target_include_directories(kernel.api INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
        ${flanterm_SOURCE_DIR}/src
)

add_executable(kernel.elf ${SOURCES} ${HEADERS} ${FLANTERM_SOURCES}
        IO/Serial.h
        IO/PortIO.h
        IO/Serial.cpp
        IO/PortIO.cpp)

target_link_libraries(kernel.elf PRIVATE kernel.api)
target_include_directories(kernel.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(kernel.elf PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${KERNEL_COMPILE_FLAGS_CXX} -fno-threadsafe-statics -freflection>
        $<$<COMPILE_LANGUAGE:C>:${KERNEL_COMPILE_FLAGS_C}>
)

target_link_options(kernel.elf PRIVATE
        -g
        -T ${CMAKE_SOURCE_DIR}/src/config/linker.ld
        -nostdlib
        -fvisibility=default
        -ggdb
        -nostartfiles
        ${LINKER_OPTIONS}
)