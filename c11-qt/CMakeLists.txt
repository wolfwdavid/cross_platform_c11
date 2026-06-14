cmake_minimum_required(VERSION 3.25)
project(c11 VERSION 0.1.0 LANGUAGES CXX C OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include build helpers
include(cmake/PlatformConfig.cmake)
include(cmake/QtSetup.cmake)
include(cmake/GhosttyIntegration.cmake)

# Main application sources
set(C11_SOURCES
    src/main.cpp
    src/app/C11Application.cpp
    src/ghostty/GhosttyRuntime.cpp
    src/ghostty/GhosttyConfig.cpp
    src/ghostty/GhosttyWidget.cpp
    src/ghostty/GhosttyKeyMapper.cpp
    src/panel/Panel.cpp
    src/panel/TerminalPanel.cpp
    src/panel/BrowserPanel.cpp
    src/panel/MarkdownPanel.cpp
    src/i18n/TranslationManager.cpp
    src/ui/KeyboardShortcutSettings.cpp
    src/ui/DragDropHandler.cpp
    src/sidebar/PortScanner.cpp
    src/agent/SkillInstaller.cpp
    src/session/SessionPersistence.cpp
    src/theme/ThemeManager.cpp
    src/mailbox/MailboxEnvelope.cpp
    src/mailbox/MailboxDispatcher.cpp
    src/metadata/SurfaceMetadataStore.cpp
    src/metadata/PaneMetadataStore.cpp
    src/metadata/GitContextResolver.cpp
    src/agent/AgentDetector.cpp
    src/sidebar/AgentChipWidget.cpp
    src/sidebar/NotificationsWidget.cpp
    src/socket/SocketServer.cpp
    src/socket/SocketProtocol.cpp
    src/socket/SocketCommandRouter.cpp
    src/ui/FindOverlay.cpp
    src/ui/StatusBar.cpp
    src/workspace/PaneLayout.cpp
    src/workspace/PaneLayoutWidget.cpp
    src/workspace/Workspace.cpp
    src/workspace/WorkspaceManager.cpp
    src/workspace/WorkspaceStackWidget.cpp
    src/sidebar/SidebarWidget.cpp
    src/ui/MainWindow.cpp
    src/platform/PlatformAbstraction.cpp
    $<$<PLATFORM_ID:Windows>:src/platform/platform_windows.cpp>
)

if(APPLE)
    list(APPEND C11_SOURCES src/ghostty/GhosttyNSViewBridge.mm)
    set_source_files_properties(src/ghostty/GhosttyNSViewBridge.mm PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )
endif()

add_executable(c11-app ${C11_SOURCES})

target_include_directories(c11-app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../  # For ghostty.h in the project root
)

target_link_libraries(c11-app PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Network
    ghostty
    ${C11_PLATFORM_LIBS}
)

target_compile_definitions(c11-app PRIVATE
    C11_VERSION="${PROJECT_VERSION}"
    $<$<CONFIG:Debug>:C11_DEBUG>
)

set_target_properties(c11-app PROPERTIES
    OUTPUT_NAME "c11"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# macOS bundle
if(APPLE)
    set_target_properties(c11-app PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "c11"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.stage11.c11"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist"
    )
endif()

# Standalone CLI binary (no Qt GUI, no Ghostty)
add_executable(c11-cli cli/c11cli.cpp)
target_link_libraries(c11-cli PRIVATE Qt6::Core Qt6::Network)
target_compile_definitions(c11-cli PRIVATE C11_VERSION="${PROJECT_VERSION}")
set_target_properties(c11-cli PROPERTIES
    OUTPUT_NAME "c11"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/cli"
)

# Tests
option(C11_BUILD_TESTS "Build unit tests" ON)
if(C11_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
