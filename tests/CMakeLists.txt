# Test harness for O2Emu
# Uses Google Test for unit and integration tests

cmake_minimum_required(VERSION 3.20)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)

# Test executable
add_executable(o2emu_tests
    test_cpu.cpp
    test_memory.cpp
    test_devices.cpp
    test_graphics.cpp
    test_firmware.cpp
    test_integration.cpp
)

target_link_libraries(o2emu_tests
    o2emu_core
    GTest::gtest
    GTest::gtest_main
    Qt6::Core
)

target_include_directories(o2emu_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../emu/include
)

# Add test
add_test(NAME o2emu_unit_tests COMMAND o2emu_tests)

# Set properties
set_target_properties(o2emu_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)