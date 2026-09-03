# GUI version using Qt6 with Fusion style

cmake_minimum_required(VERSION 3.20)

# Qt6 Widgets and Gui already found in parent CMakeLists.txt

add_executable(o2emu-gui
    main.cpp
    mainwindow.cpp
    framebufferwidget.cpp
    debuggerwidget.cpp
    resources.qrc
)

target_link_libraries(o2emu-gui PRIVATE
    o2emu_core
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)

set_target_properties(o2emu-gui PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Resources
qt6_add_resources(o2emu-gui "resources.qrc")