# ARM 64-bit (aarch64) Cross Compilation Toolchain File
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross compiler prefixes
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CMAKE_STRIP aarch64-linux-gnu-strip CACHE INTERNAL "strip tool")

set(AARCH64_FLAGS "-march=armv8-a -mtune=cortex-a53 -Wall -O2")
set(CMAKE_C_FLAGS_INIT "${AARCH64_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${AARCH64_FLAGS}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
