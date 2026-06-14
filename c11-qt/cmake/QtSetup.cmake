# Find and configure Qt 6

find_package(Qt6 6.7 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets)
qt_standard_project_setup()

message(STATUS "Qt6 found: ${Qt6_DIR}")
message(STATUS "Qt6 version: ${Qt6_VERSION}")
