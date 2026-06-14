find_package(Qt6 REQUIRED COMPONENTS Test)

# GhosttyConfig unit tests (no Ghostty runtime needed)
add_executable(test_ghostty_config
    tst_GhosttyConfig.cpp
    ../src/ghostty/GhosttyConfig.cpp
)

target_include_directories(test_ghostty_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../  # For ghostty.h
)

target_link_libraries(test_ghostty_config PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME GhosttyConfigTest COMMAND test_ghostty_config)

# GhosttyKeyMapper unit tests
add_executable(test_key_mapper
    tst_GhosttyKeyMapper.cpp
    ../src/ghostty/GhosttyKeyMapper.cpp
)

target_include_directories(test_key_mapper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
)

target_link_libraries(test_key_mapper PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME GhosttyKeyMapperTest COMMAND test_key_mapper)

# Platform abstraction tests
add_executable(test_platform
    tst_Platform.cpp
    ../src/platform/PlatformAbstraction.cpp
    $<$<PLATFORM_ID:Windows>:../src/platform/platform_windows.cpp>
)

target_include_directories(test_platform PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_platform PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME PlatformTest COMMAND test_platform)

# PaneLayout unit tests
add_executable(test_pane_layout
    tst_PaneLayout.cpp
    ../src/workspace/PaneLayout.cpp
)

target_include_directories(test_pane_layout PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_pane_layout PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME PaneLayoutTest COMMAND test_pane_layout)

# WorkspaceManager unit tests (stub mode, no Ghostty linking)
add_executable(test_workspace_manager
    tst_WorkspaceManager.cpp
    ../src/workspace/WorkspaceManager.cpp
    ../src/workspace/Workspace.cpp
    ../src/workspace/PaneLayout.cpp
    ../src/panel/Panel.cpp
    ../src/panel/TerminalPanel.cpp
    ../src/panel/BrowserPanel.cpp
    ../src/panel/MarkdownPanel.cpp
    ../src/ghostty/GhosttyWidget.cpp
    ../src/ghostty/GhosttyKeyMapper.cpp
    ../src/ghostty/GhosttyRuntime.cpp
    ../src/ghostty/GhosttyConfig.cpp
)

target_include_directories(test_workspace_manager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
)

target_compile_definitions(test_workspace_manager PRIVATE C11_GHOSTTY_STUB)

target_link_libraries(test_workspace_manager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Test
)

add_test(NAME WorkspaceManagerTest COMMAND test_workspace_manager)

# SocketProtocol unit tests
add_executable(test_socket_protocol
    tst_SocketProtocol.cpp
    ../src/socket/SocketProtocol.cpp
)

target_include_directories(test_socket_protocol PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_socket_protocol PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME SocketProtocolTest COMMAND test_socket_protocol)

# MetadataStore unit tests
add_executable(test_metadata_store
    tst_MetadataStore.cpp
    ../src/metadata/SurfaceMetadataStore.cpp
)

target_include_directories(test_metadata_store PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_metadata_store PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME MetadataStoreTest COMMAND test_metadata_store)

# SessionPersistence unit tests
add_executable(test_session_persistence
    tst_SessionPersistence.cpp
    ../src/session/SessionPersistence.cpp
    ../src/workspace/WorkspaceManager.cpp
    ../src/workspace/Workspace.cpp
    ../src/workspace/PaneLayout.cpp
    ../src/panel/Panel.cpp
    ../src/panel/TerminalPanel.cpp
    ../src/panel/BrowserPanel.cpp
    ../src/panel/MarkdownPanel.cpp
    ../src/ghostty/GhosttyWidget.cpp
    ../src/ghostty/GhosttyKeyMapper.cpp
    ../src/ghostty/GhosttyRuntime.cpp
    ../src/ghostty/GhosttyConfig.cpp
    ../src/platform/PlatformAbstraction.cpp
    $<$<PLATFORM_ID:Windows>:../src/platform/platform_windows.cpp>
)

target_include_directories(test_session_persistence PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
)

target_compile_definitions(test_session_persistence PRIVATE C11_GHOSTTY_STUB)

target_link_libraries(test_session_persistence PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Test
)

add_test(NAME SessionPersistenceTest COMMAND test_session_persistence)

# MailboxEnvelope unit tests
add_executable(test_mailbox_envelope
    tst_MailboxEnvelope.cpp
    ../src/mailbox/MailboxEnvelope.cpp
)

target_include_directories(test_mailbox_envelope PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_mailbox_envelope PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME MailboxEnvelopeTest COMMAND test_mailbox_envelope)
