# CLI (Terminal) version for debugging/logging

cmake_minimum_required(VERSION 3.20)

add_executable(o2emu-cli
    main.cpp
)

target_link_libraries(o2emu-cli PRIVATE
    o2emu_core
    Qt6::Core
)

set_target_properties(o2emu-cli PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)