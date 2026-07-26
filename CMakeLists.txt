cmake_minimum_required(VERSION 3.31)
project(BorealOS LANGUAGES CXX ASM)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(COMPILE_FLAGS
        -mcmodel=kernel
        -mno-red-zone
        -m64
        -mabi=sysv
        -mno-sse2
        -mno-mmx
        -mno-avx
        -mno-avx512f
)

set(LINKER_OPTIONS
        -mcmodel=kernel
        -mno-red-zone
        -m64
        -Wl,--hash-style=sysv
        -mabi=sysv
)

set(KERNEL_COMPILE_FLAGS_COMMON
        -O0
        -ffreestanding
        -fno-asynchronous-unwind-tables
        -fno-stack-protector
        -Wall -Wextra -Werror -Wpedantic
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-function
        ${COMPILE_FLAGS}
)

set(KERNEL_COMPILE_FLAGS_CXX
        ${KERNEL_COMPILE_FLAGS_COMMON}
        -fno-exceptions
        -fno-rtti
        -fno-common
        -Wpedantic
        -Wno-vla
        -Wno-zero-length-array
        -Wno-gnu-flexible-array-member
        -fno-use-cxa-atexit
)

set(KERNEL_COMPILE_FLAGS_C
        ${KERNEL_COMPILE_FLAGS_COMMON}
        -fno-exceptions
        -Wno-vla
        -Wno-zero-length-array
)

set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")
set(CMAKE_C_STANDARD_LIBRARIES "")
set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_C_IMPLICIT_LINK_DIRECTORIES "")

add_subdirectory(src/kernel)

add_custom_target(CopyKernel ALL
        DEPENDS kernel.elf
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel.elf> ${CMAKE_SOURCE_DIR}/src/config/boot/kernel.elf
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_command(TARGET CopyKernel POST_BUILD
        COMMAND sh -c "nm -f posix ${CMAKE_SOURCE_DIR}/src/config/boot/kernel.elf | awk '{print \$1 \" \" \$3}' | sort -k 1,1 > ${CMAKE_SOURCE_DIR}/ramfs/kernel.sym"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        VERBATIM
)

add_custom_target(Kernel ALL DEPENDS CopyKernel)