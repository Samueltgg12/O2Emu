add_executable(o2emu_tests
    test_cpu_interface.cpp
)

target_link_libraries(o2emu_tests PRIVATE o2emu_core)

add_test(NAME cpu_interface_tests COMMAND o2emu_tests)
