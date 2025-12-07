cmake_minimum_required(VERSION 3.10)
project(green_threads VERSION 0.1.0 LANGUAGES CXX ASM)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add the library
add_library(green_threads
    src/core/thread.cpp
    src/core/scheduler.cpp
    src/runtime/runtime.cpp
    src/arch/x86_64/context.S
)

target_include_directories(green_threads PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Add examples
add_executable(example_basic examples/basic.cpp)
add_executable(example_yield examples/yield_example.cpp)
add_executable(example_multiple examples/multiple_threads.cpp)

target_link_libraries(example_basic PRIVATE green_threads)
target_link_libraries(example_yield PRIVATE green_threads)
target_link_libraries(example_multiple PRIVATE green_threads)
