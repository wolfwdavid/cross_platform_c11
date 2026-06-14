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
