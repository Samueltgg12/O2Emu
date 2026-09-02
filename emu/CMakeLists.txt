cmake_minimum_required(VERSION 3.20)
project(O2Emu VERSION 0.1.0 LANGUAGES CXX)

# C++20 required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build types
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fsanitize=address,undefined")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Core library
add_library(o2emu_core STATIC
    src/memory_map.cpp
    src/logging.cpp
    src/tracing.cpp
    src/bus.cpp
    src/cpu_interface.cpp
    src/crime.cpp
    src/gbe.cpp
    src/mace.cpp
    src/vice.cpp
)

# Main emulator executable
add_executable(o2emu
    src/main.cpp
)
target_link_libraries(o2emu PRIVATE o2emu_core)

# Unit tests
enable_testing()
add_subdirectory(tests)

# Install targets
install(TARGETS o2emu DESTINATION bin)
install(TARGETS o2emu_core DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/o2emu)