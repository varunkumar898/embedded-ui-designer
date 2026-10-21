cmake_minimum_required(VERSION 3.20)
project(TestDashboard C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Include Generated and User Source Directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/user
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/bindings
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/target
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/assets
)

file(GLOB_RECURSE GENERATED_SOURCES
    "generated/*.c"
    "generated/*.cpp"
)

file(GLOB USER_SOURCES
    "user/*.c"
    "user/*.cpp"
)

add_executable(TestDashboard
    ${GENERATED_SOURCES}
    ${USER_SOURCES}
)

# Target Compiler Optimizations for MCU
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(TestDashboard PRIVATE -Wall -Wextra -Os -ffunction-sections -fdata-sections)
    target_link_options(TestDashboard PRIVATE -Wl,--gc-sections)
endif()
