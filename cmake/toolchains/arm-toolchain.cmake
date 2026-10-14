# ARM 32-bit Bare-metal / Embedded Linux Toolchain File
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compiler
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy CACHE INTERNAL "objcopy tool")
set(CMAKE_OBJDUMP arm-none-eabi-objdump CACHE INTERNAL "objdump tool")
set(CMAKE_SIZE arm-none-eabi-size CACHE INTERNAL "size tool")

# Target flags for Cortex-M0/M3/M4
set(ARM_FLAGS "-mcpu=cortex-m0 -mthumb -mfloat-abi=soft -Wall -fdata-sections -ffunction-sections")
set(CMAKE_C_FLAGS_INIT "${ARM_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${ARM_FLAGS} -fno-rtti -fno-exceptions")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
