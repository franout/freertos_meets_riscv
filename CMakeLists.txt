cmake_minimum_required(VERSION 3.15)


set(CMAKE_C_FLAGS "$ENV{OPTIMIZATION_FLAGS} -static -march=rv32i_zicsr  -mabi=ilp32 -mcmodel=medlow -nostartfiles   ")
set(CMAKE_CXX_FLAGS " $ENV{OPTIMIZATION_FLAGS} -static -march=rv32i_zicsr  -mabi=ilp32 -mcmodel=medlow -nostartfiles ")
set(CMAKE_EXE_LINKER_FLAGS "$ENV{OPTIMIZATION_FLAGS} -T ./platform/linker.ld -static -march=rv32i_zicsr  -mabi=ilp32 -mcmodel=medlow -nostartfiles")
set(ASM_OPTIONS "-x assembler-with-cpp ")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} ${ASM_OPTIONS}" )

project($ENV{program}.elf
        LANGUAGES C CXX ASM)

set(FREERTOS_KERNEL_PATH "./freeRTOS_kernel/")


# Select the heap port.  values between 1-4 will pick a heap.
set(FREERTOS_HEAP "1" CACHE STRING "" FORCE)

# Select the native compile PORT
set(FREERTOS_PORT GCC_RISC_V_GENERIC   CACHE STRING "GCC_RISC_V_GENERIC" FORCE)


set(FREERTOS_RISCV_EXTENSION RISCV_MTIME_CLINT_no_extensions CACHE STRING "RISCV_MTIME_CLINT_no_extensions" FORCE)


# Add the freertos_config for FreeRTOS-Kernel
add_library(freertos_config INTERFACE)

target_include_directories(freertos_config
    INTERFACE
    "./"
    "./RISCV-platform"
)
# Adding the FreeRTOS-Kernel subdirectory
add_subdirectory(${FREERTOS_KERNEL_PATH} FreeRTOS-Kernel)

########################################################################
# Overall Compile Options
# Note the compile option strategy is to error on everything and then
# Per library opt-out of things that are warnings/errors.
# This ensures that no matter what strategy for compilation you take, the
# builds will still occur.
#
# Only tested with GNU and Clang.
# Other options are https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html#variable:CMAKE_%3CLANG%3E_COMPILER_ID
# Naming of compilers translation map:
#
#   FreeRTOS    | CMake
#   -------------------
#   CCS         | ?TBD?
#   GCC         | GNU, Clang, *Clang Others?
#   IAR         | IAR
#   Keil        | ARMCC
#   MSVC        | MSVC # Note only for MinGW?
#   Renesas     | ?TBD?


target_compile_options(freertos_kernel PRIVATE
    ### Gnu/Clang C Options
    $<$<COMPILE_LANG_AND_ID:C,GNU>:-fdiagnostics-color=always>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-fcolor-diagnostics>

    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-march=rv32i_zicsr >
    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-mabi=ilp32>
    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-mcmodel=medlow>
    
    #$<$<COMPILE_LANG_AND_ID:C,Clang,GNU>: $ENV{OPTIMIZATION_FLAGS}>

    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wall>
    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wextra>
    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wpedantic>
    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Werror>
    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wconversion>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Weverything>

    # Suppressions required to build clean with clang.
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-unused-macros>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-padded>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-missing-variable-declarations>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-covered-switch-default>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-cast-align>
    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-T ./RISCV-platform/linker.ld>
    )



add_executable(${PROJECT_NAME}
    ./RISCV-platform/riscv-csr.h
    ./RISCV-platform/startup.c
    ./RISCV-platform/timer.h
    ./RISCV-platform/timer.c
    main.c
)


target_link_libraries(${PROJECT_NAME}  freertos_kernel  )
 
set_property(TARGET freertos_kernel PROPERTY C_STANDARD 99)