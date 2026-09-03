# O2Emu Core Library
# This is the core emulator library containing CPU, memory, devices, etc.

cmake_minimum_required(VERSION 3.20)

# Core library
add_library(o2emu_core STATIC
    src/cpu/cpu.cpp
    src/cpu/mips_r5000.cpp
    src/cpu/cp0.cpp
    src/memory/memory.cpp
    src/memory/address_space.cpp
    src/memory/crime.cpp
    src/memory/mre.cpp
    src/devices/device.cpp
    src/devices/mace/mace.cpp
    src/devices/mace/mace_pci.cpp
    src/devices/mace/mace_ethernet.cpp
    src/devices/mace/mace_audio.cpp
    src/devices/mace/mace_isa.cpp
    src/devices/scsicontroller.cpp
    src/devices/rtc.cpp
    src/devices/uart.cpp
    src/devices/ps2.cpp
    src/firmware/prom.cpp
    src/firmware/prom_loader.cpp
    src/system/bus.cpp
    src/system/interrupts.cpp
    src/system/timer.cpp
    src/logging/logger.cpp
    src/graphics/microprocessor.cpp
    src/graphics/ice.cpp
    src/graphics/display_engine.cpp
    src/graphics/framebuffer.cpp
)

target_include_directories(o2emu_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(o2emu_core PUBLIC
    Qt6::Core
)

# Set properties
set_target_properties(o2emu_core PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)