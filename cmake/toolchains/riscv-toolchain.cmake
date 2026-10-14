# RISC-V 32/64-bit Cross Compilation Toolchain File
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

set(CMAKE_C_COMPILER riscv64-unknown-elf-gcc)
set(CMAKE_CXX_COMPILER riscv64-unknown-elf-g++)
set(CMAKE_ASM_COMPILER riscv64-unknown-elf-gcc)
set(CMAKE_OBJCOPY riscv64-unknown-elf-objcopy CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE riscv64-unknown-elf-size CACHE INTERNAL "size tool")

set(RISCV_FLAGS "-march=rv32imc -mabi=ilp32 -Wall -fdata-sections -ffunction-sections")
set(CMAKE_C_FLAGS_INIT "${RISCV_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${RISCV_FLAGS} -fno-rtti -fno-exceptions")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
