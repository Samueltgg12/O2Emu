cmake_minimum_required(VERSION 3.20)
project(O2Emu
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "SGI O2 (IP32) Emulator"
)

# C++20 standard
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
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
endif()

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# Set Qt to use Fusion style
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/emu/include)

# Subdirectories
add_subdirectory(emu)
add_subdirectory(cli)
add_subdirectory(gui)
add_subdirectory(tests)

# Install targets (optional)
# install(TARGETS o2emu-cli o2emu-gui DESTINATION bin)