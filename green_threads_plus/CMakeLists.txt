cmake_minimum_required(VERSION 3.10)
project(green_threads_plus C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g -pthread")

# Source files
set(SOURCES
    src/green_thread.c
    src/scheduler.c
    src/stack_alloc.c
    src/sync.c
    src/timers.c
    src/pthread_adapter.c
    src/http_server.c
    src/metrics.c
    src/iointeg.c
    src/aura.c
    src/dual_run.c
    src/workloads.c
)

# Include directories
include_directories(include src)

# Check for libraries
find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(LIBMICROHTTPD libmicrohttpd)
    pkg_check_modules(LIBWEBSOCKETS libwebsockets)
endif()

# Fallback or manual paths could be added here
# For now, we assume they are available or we might need to mock if missing for compilation on Windows without them.

# Main library
add_library(gthread_lib STATIC ${SOURCES})
target_link_libraries(gthread_lib ${LIBMICROHTTPD_LIBRARIES} ${LIBWEBSOCKETS_LIBRARIES})

# Main App
add_executable(green_threads_app src/demo_cli.c)
target_link_libraries(green_threads_app gthread_lib pthread)

# Build Examples (Optional, can be added later)
# Build Examples (Optional, can be added later)
file(GLOB EXAMPLE_SOURCES examples/*.c)
foreach(ex ${EXAMPLE_SOURCES})
    get_filename_component(ex_name ${ex} NAME_WE)
    add_executable(${ex_name} ${ex})
    target_link_libraries(${ex_name} gthread_lib pthread)
endforeach()
